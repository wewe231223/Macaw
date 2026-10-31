/**
 * @file Gizmo.cpp
 * @brief 이동, 회전과 스케일 기즈모의 화면 질의와 드래그를 구현합니다.
 * @ingroup Editor
 */

#include "Editor/Gizmo.h"
#include "Engine/EditorMesh.h"
#include "Engine/MeshPicking.h"
#include "Engine/UWorld.h"
#include <limits>
#include "Editor/FEditorViewportClient.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/FWorldContext.h"
#include "Core/Math.h"
#include "Engine/AActor.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FTransform.h"
#include "Core/Math/FRay.h"
#include "Engine/FSceneView.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/FViewportInput.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/FObjectRegistry.h"
#include "Editor/FEditorContext.h"
#include "Core/Literals.h"

#include <algorithm>
#include <cmath>

using namespace Core::Literals;

namespace Editor {

    /**
     * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
     */
    namespace {
        /**
         * @brief 기즈모의 기본 X·Y·Z 단위 축입니다.
         */
        const std::array<Core::FVector, 3> UnitAxes{{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

        /**
         * @brief 기즈모 X·Y·Z 축의 표시 색상입니다.
         */
        const std::array<Core::FVector, 3> AxisColors{{{1.0f, 0.28f, 0.22f}, {0.35f, 0.95f, 0.45f}, {0.32f, 0.58f, 1.0f}}};

        /**
         * @brief 지정한 축과 수직인 안정적인 단위 벡터를 구합니다.
         * @param[in] Axis 수직 벡터를 구할 기준 축입니다.
         * @return Axis와 수직인 단위 벡터입니다.
         */
        Core::FVector Perpendicular(const Core::FVector& Axis) {
            return Axis.Cross(std::abs(Axis.z) < 0.8f ? Core::FVector{0.0f, 0.0f, 1.0f} : Core::FVector{0.0f, 1.0f, 0.0f}).Normalized();
        }

        /**
         * @brief 세 기저 축과 원점으로 변환 행렬을 구성합니다.
         * @param[in] X 행렬의 X 기저 축입니다.
         * @param[in] Y 행렬의 Y 기저 축입니다.
         * @param[in] Z 행렬의 Z 기저 축입니다.
         * @param[in] Position 기저 행렬의 월드 원점입니다.
         * @return 기저 축과 평행이동을 반영한 행렬입니다.
         */
        Core::FMatrix Basis(const Core::FVector& X, const Core::FVector& Y, const Core::FVector& Z, const Core::FVector& Position) {
            Core::FMatrix Result{Core::FMatrix::Identity()};
            const std::array<Core::FVector, 4> Rows{X, Y, Z, Position};

            for (Core::Int32 Index{}; Index < 4; ++Index) {
                Result(Index, 0) = Rows[Index].x;
                Result(Index, 1) = Rows[Index].y;
                Result(Index, 2) = Rows[Index].z;
            }

            return Result;
        }

        /**
         * @brief 기즈모 핸들이 주어진 로컬 축에 영향을 주는지 검사합니다.
         * @param[in] Handle 기즈모 핸들 번호입니다.
         * @param[in] Axis 검사할 축의 인덱스입니다.
         * @return 조건을 만족하거나 변경을 적용하면 true입니다.
         */
        bool Affects(Core::Int32 Handle, Core::Int32 Axis) {
            return Handle == Axis || (Handle == 3 && Axis != 2) || (Handle == 4 && Axis != 1) || (Handle == 5 && Axis != 0);
        }

        /**
         * @brief 스냅이 켜져 있고 간격이 유효하면 값을 가장 가까운 배수로 반올림합니다.
         * @param[in] Value 스냅을 적용할 값입니다.
         * @param[in] Step 스냅 간격입니다.
         * @param[in] Enabled 스냅을 활성화할지 여부입니다.
         * @return 유효한 스냅을 적용한 값 또는 원본 값입니다.
         */
        float Snap(float Value, float Step, bool Enabled) {
            return Enabled && std::isfinite(Step) && Step > 0.0f ? std::round(Value / Step) * Step : Value;
        }

    }

    /**
     * @brief 에디터 편집 경로와 클라이언트가 제공하는 변환 도구 상태를 연결합니다.
     */
    Editor::FTransformGizmo::FTransformGizmo(Editor::FEditorContext* Context, const Editor::FEditorViewportClient* Client)
		: mContext{Context},
		  mClient{Client} {
    }

    void Editor::FTransformGizmo::SetEditorContext(Editor::FEditorContext* Context) {
        if (Context == nullptr || Context == mContext) {
            return;
        }

        Cancel();
        mContext = Context;
        mHasPivot = false;
        mPivotRevision = 0;
    }

    /**
     * @brief 선택한 좌표 공간에 맞는 기즈모 단위 축을 구합니다.
     */
    Core::FVector Editor::FTransformGizmo::GetAxis(const Core::FTransform& Transform, Core::Int32 Index) const {
        return mClient->GetGizmoSpace() == Editor::EGizmoSpace::Local ? Transform.TransformVectorNoScale(UnitAxes[Index]) : UnitAxes[Index];
    }

    /**
     * @brief 크기를 반환합니다.
     */
    float Editor::FTransformGizmo::GetSize(const Engine::FSceneView& View, const Core::FVector& Pivot) const {
        const float Depth{(Pivot - View.mLocation).Dot(View.mForward)};
        const float Size{std::isfinite(mSettings.mSize) ? std::clamp(mSettings.mSize, 0.5f, 2.0f) : 1.0f};

        if (View.mOrthographic) {
            return 2.0f / (View.mProjection(1, 1) * std::max(View.mSize.y, 1.0f)) * Size;
        }

        return Depth > 0.05f ? 2.0f * Depth * Core::Math::Tan(View.mFieldOfView * 0.5_deg) / std::max(View.mSize.y, 1.0f) * Size : 0.0f;
    }

    /**
     * @brief 그리는 메시의 CPU 레이 교차 판정으로 선택 액터의 기즈모 HitProxy를 구합니다.
     */
    Engine::FHitProxy Editor::FTransformGizmo::HitTest(const Core::FVector2& Mouse, const Engine::FSceneView& View) const {
        if (!mContext->CanEditWorld()) {
            return {};
        }

        Core::FRay Ray{};

        if (!View.Deproject(Mouse, Ray)) {
            return {};
        }

        Engine::FPrimitiveDrawInterface Shapes{};
        BuildHandles(View, &Shapes);

        Engine::FHitProxy Result{};
        Core::Uint32 Priority{std::numeric_limits<Core::Uint32>::max()};
        float Closest{std::numeric_limits<float>::max()};

        for (const Engine::FDrawMesh& Shape : Shapes.GetMeshes()) {
            if (Shape.mHitProxy.mType == Engine::EHitProxyType::None || Shape.mPriority > Priority) {
                continue;
            }

            float Distance{Shape.mPriority == Priority ? Closest : std::numeric_limits<float>::max()};

            if (Engine::RaycastMesh(&Engine::GetEditorMesh(Shape.mMesh), Shape.mWorld, Ray, Distance)) {
                Result = Shape.mHitProxy;
                Closest = Distance;
                Priority = Shape.mPriority;
            }
        }

        return Result;
    }

    /**
     * @brief 선택 축과 대상의 시작 변환을 저장하고 드래그 평면을 구성합니다.
     */
    bool Editor::FTransformGizmo::BeginDrag(Core::Int32 Axis, const Engine::FViewportInput* Input, const Engine::FSceneView& View) {
        Engine::USceneComponent* Target{GetPrimaryTarget()};

        if (Input == nullptr || Target == nullptr || !mContext->CanEditWorld() || Axis < 0 || Axis > 7) {
            return false;
        }

        if (!CaptureTargets()) {
            return false;
        }

        mStartTransform = Target->GetTransform();
        mStartPivot = GetPivot();
        mOriginalPivot = mPivot;
        mHadPivot = mHasPivot && mPivotRevision == mContext->GetSelectionRevision();
        mStartMouse = Input->mMouse;
        mDragView = View;
        mDragSize = GetSize(View, mStartPivot);
        mDragMode = mClient->GetGizmoMode();
        mPivotOnly = Input->mMiddlePressed && mDragMode == Editor::EGizmoMode::Translate;
        mDuplicatePending = Input->mAlt && !mPivotOnly && mDragMode == Editor::EGizmoMode::Translate;
        mOriginalSelection = mContext->GetSelectedActors();
        mOriginalObject = mContext->GetSelectedObject() != nullptr ? mContext->GetSelectedObject()->GetHandle() : CoreUObject::FObjectHandle{};
        mSelectionRevision = mContext->GetSelectionRevision();
        mTranslation = {};
        mRawAngle = 0.0f;
        mAppliedAngle = 0.0f;
        mDuplicates.clear();

        for (Core::Int32 Index{}; Index < 3; ++Index) {
            mDragAxes[Index] = GetAxis(mStartTransform, Index);
        }

        mAxis = Axis < 3 ? mDragAxes[Axis] : View.mForward;
        mPlaneNormal = View.mForward;

        if (Axis >= 3 && Axis <= 5) {
            mPlaneNormal = Axis == 3 ? mDragAxes[0].Cross(mDragAxes[1]) : Axis == 4 ? mDragAxes[0].Cross(mDragAxes[2]) : mDragAxes[1].Cross(mDragAxes[2]);
            mAxis = {};

            for (Core::Int32 Index{}; Index < 3; ++Index) {
                if (Affects(Axis, Index)) {
                    mAxis += mDragAxes[Index];
                }
            }
        } else if (mDragMode == Editor::EGizmoMode::Rotate) {
            mPlaneNormal = mAxis;
        } else if (Axis < 3) {
            mPlaneNormal = View.mForward.ProjectedOnPlane(mAxis);
        }

        if (!mPlaneNormal.Normalize()) {
            return false;
        }

        Core::FVector2 Start{};
        Core::FVector2 End{};

        if (!Core::Math::Project(mStartPivot, View.mViewProjection, View.mSize, Start) || !Core::Math::Project(mStartPivot + mAxis * mDragSize * 100.0f, View.mViewProjection, View.mSize, End)) {
            return false;
        }

        mScreenAxis = End - Start;

        const float Length{std::sqrt(mScreenAxis.x * mScreenAxis.x + mScreenAxis.y * mScreenAxis.y)};
        mScreenAxis = Axis >= 6 ? Core::FVector2{0.70710678f, -0.70710678f} : Length > 0.00001f ? mScreenAxis / Length : Core::FVector2{};

        Core::FRay Ray{};

        if (!View.Deproject(Input->mMouse, Ray) || mDragSize <= 0.0f) {
            return false;
        }

        mScreenFallback = std::abs(Ray.mDirection.Dot(mPlaneNormal)) < 0.12f || !Core::Math::IntersectPlane(Ray, mStartPivot, mPlaneNormal, mStartIntersection);

        if (mDragMode == Editor::EGizmoMode::Rotate) {
            mStartDirection = mScreenFallback ? Ray.mOrigin + Ray.mDirection * (mStartPivot - Ray.mOrigin).Dot(Ray.mDirection) - mStartPivot : mStartIntersection - mStartPivot;
            mStartDirection = mStartDirection.ProjectedOnPlane(mPlaneNormal).Normalized();

            if (mStartDirection.IsNearlyZero()) {
                return false;
            }

            mPreviousDirection = mStartDirection;

            const Core::FVector Tangent{mPlaneNormal.Cross(mStartDirection)};

            if (Core::Math::Project(mStartPivot + mStartDirection * mDragSize * 74.0f, View.mViewProjection, View.mSize, Start) && Core::Math::Project(mStartPivot + (mStartDirection * 74.0f + Tangent * 10.0f) * mDragSize, View.mViewProjection, View.mSize, End)) {
                mScreenAxis = End - Start;

                const float TangentLength{std::sqrt(mScreenAxis.x * mScreenAxis.x + mScreenAxis.y * mScreenAxis.y)};

                if (TangentLength > 0.00001f) {
                    mScreenAxis /= TangentLength;
                }
            }
        }

        mWorldHandle = mContext->GetWorldHandle();
        mActiveAxis = Axis;

        return true;
    }

    /**
     * @brief 마우스 이동량을 활성 모드의 변환으로 바꾸어 선택한 액터에 적용합니다.
     */
    void Editor::FTransformGizmo::UpdateDrag(const Engine::FViewportInput* Input) {
        Core::FMatrix CameraWorld{};

        if (!mDragView.mView.TryInverse(CameraWorld)) {
            return;
        }

        const Core::FVector Right{CameraWorld.GetAxis(0)};
        const Core::FVector Up{CameraWorld.GetAxis(1)};

        if (Input == nullptr || !IsDragging()) {
            return;
        }

        const Core::FVector2 MouseDelta{Input->mMouse - mStartMouse};

        if (!std::isfinite(MouseDelta.x) || !std::isfinite(MouseDelta.y)) {
            return;
        }

        if (mDuplicatePending && MouseDelta.x * MouseDelta.x + MouseDelta.y * MouseDelta.y > 9.0f) {
            mDuplicates = mContext->DuplicateSelectedActors();

            if (mDuplicates.empty()) {
                Cancel();
                return;
            }

            mSelectionRevision = mContext->GetSelectionRevision();
            mPivotRevision = mSelectionRevision;

            if (!CaptureTargets()) {
                Cancel();
                return;
            }

            mDuplicatePending = false;
        }

        for (const FTargetState& State : mTargets) {
            if (mContext->ResolveObject(State.mObject) == nullptr) {
                Cancel();
                return;
            }
        }

        Core::FRay Ray{};

        if (!mDragView.Deproject(Input->mMouse, Ray)) {
            return;
        }

        const float ScreenMovement{MouseDelta.x * mScreenAxis.x + MouseDelta.y * mScreenAxis.y};

        if (mDragMode == Editor::EGizmoMode::Rotate) {
            if (mScreenFallback) {
                mRawAngle = ScreenMovement / (74.0f * std::clamp(mSettings.mSize, 0.5f, 2.0f));
            } else {
                Core::FVector Intersection{};

                if (!Core::Math::IntersectPlane(Ray, mStartPivot, mPlaneNormal, Intersection)) {
                    return;
                }

                const Core::FVector Direction{(Intersection - mStartPivot).ProjectedOnPlane(mPlaneNormal).Normalized()};

                if (Direction.IsNearlyZero()) {
                    return;
                }

                mRawAngle += std::atan2(mPlaneNormal.Dot(mPreviousDirection.Cross(Direction)), std::clamp(mPreviousDirection.Dot(Direction), -1.0f, 1.0f));
                mPreviousDirection = Direction;
            }

            mAppliedAngle = Snap(mRawAngle, mSettings.mRotationStep * 1_deg / 1_rad, mSettings.mRotationSnap);

            const Core::FQuat Rotation{mPlaneNormal, mAppliedAngle};

            for (const FTargetState& State : mTargets) {
                Core::FTransform Desired{State.mTransform};
                Desired.mLocation = mStartPivot + Rotation * (Desired.mLocation - mStartPivot);
                Desired.mRotation = (Rotation * Desired.mRotation).Normalized();

                if (!mContext->ApplyTransform(State.mObject, Desired, true)) {
                    Cancel();
                    return;
                }
            }

            return;
        }

        if (mDragMode == Editor::EGizmoMode::Scale) {
            const float Delta{Snap(ScreenMovement / 100.0f, mSettings.mScaleStep, mSettings.mScaleSnap)};
            const float Uniform{std::max(0.001f, 1.0f + Delta)};
            const auto Primary{mStartTransform.mScale.ToArray()};
            std::array<float, 3> Factors{1.0f, 1.0f, 1.0f};

            for (Core::Int32 Index{}; Index < 3; ++Index) {
                const float Value{Primary[Index] + Delta};
                Factors[Index] = mActiveAxis == 7 ? Uniform : Affects(mActiveAxis, Index) && std::abs(Primary[Index]) > 0.000001f ? (Primary[Index] < 0.0f ? std::min(-0.001f, Value) : std::max(0.001f, Value)) / Primary[Index] : 1.0f;
            }

            for (const FTargetState& State : mTargets) {
                Core::FTransform Desired{State.mTransform};
                auto Scale{Desired.mScale.ToArray()};

                for (Core::Int32 Index{}; Index < 3; ++Index) {
                    const float Value{mActiveAxis == 7 ? Scale[Index] * Uniform : Scale[Index] + (Affects(mActiveAxis, Index) ? Delta : 0.0f)};
                    Scale[Index] = Scale[Index] < 0.0f ? std::min(-0.001f, Value) : std::max(0.001f, Value);
                }

                Desired.mScale = {Scale[0], Scale[1], Scale[2]};

                const Core::FVector Offset{State.mTransform.mLocation - mStartPivot};

                Desired.mLocation = mStartPivot;

                for (Core::Int32 Index{}; Index < 3; ++Index) {
                    Desired.mLocation += mDragAxes[Index] * Offset.Dot(mDragAxes[Index]) * Factors[Index];
                }

                if (!mContext->ApplyTransform(State.mObject, Desired, true)) {
                    Cancel();
                    return;
                }
            }

            return;
        }

        Core::FVector Delta{};
        Core::FVector Movement{};

        if (mScreenFallback) {
            Movement = mActiveAxis < 3 ? mDragAxes[mActiveAxis] * ScreenMovement * mDragSize : (Right * MouseDelta.x - Up * MouseDelta.y) * mDragSize;
        } else {
            Core::FVector Intersection{};

            if (!Core::Math::IntersectPlane(Ray, mStartPivot, mPlaneNormal, Intersection)) {
                return;
            }

            Movement = Intersection - mStartIntersection;
        }

        for (Core::Int32 Index{}; Index < 3; ++Index) {
            const Core::FVector Axis{mActiveAxis == 6 ? UnitAxes[Index] : mDragAxes[Index]};

            if (mActiveAxis == 6 || Affects(mActiveAxis, Index)) {
                Delta += Axis * Snap(Movement.Dot(Axis), mSettings.mTranslationStep, mSettings.mTranslationSnap);
            }
        }

        if (!Delta.IsFinite()) {
            return;
        }

        mTranslation = Delta;

        if (mPivotOnly || mHadPivot) {
            mPivot = mStartPivot + Delta;
            mHasPivot = true;
            mPivotRevision = mContext->GetSelectionRevision();
        }

        if (mPivotOnly) {
            return;
        }

        for (const FTargetState& State : mTargets) {
            Core::FTransform Desired{State.mTransform};
            Desired.mLocation += Delta;

            if (!mContext->ApplyTransform(State.mObject, Desired, true)) {
                Cancel();
                return;
            }
        }
    }

    /**
     * @brief 현재 프레임의 뷰포트 입력을 처리합니다.
     */
    bool Editor::FTransformGizmo::Input(const Engine::FViewportInput& Input, const Engine::FSceneView& View, const Engine::FHitProxy& Hit) {
        if (IsDragging()) {
            if (Input.mEscape || mContext->GetWorldHandle() != mWorldHandle || !mContext->CanEditWorld() || mContext->GetSelectionRevision() != mSelectionRevision) {
                Cancel();
            } else if (mPivotOnly ? Input.mMiddleReleased || !Input.mMiddleDown : Input.mLeftReleased || !Input.mLeftDown) {
                if (mPivotOnly ? Input.mMiddleReleased : Input.mLeftReleased) {
                    UpdateDrag(&Input);
                }

                EndDrag(false);
            } else {
                UpdateDrag(&Input);
            }

            return true;
        }

        const Engine::USceneComponent* Target{GetPrimaryTarget()};
        const Engine::AActor* Actor{Target != nullptr ? Target->GetOwner() : nullptr};
        mHoveredAxis = Actor != nullptr && mContext->CanEditWorld() && Input.mHovered && !Input.mRightDown && Hit.mType == Engine::EHitProxyType::GizmoAxis && Hit.mObject == Actor->GetHandle() && Hit.mAxis >= 0 && Hit.mAxis <= 7 ? Hit.mAxis : -1;

        const bool Pressed{Input.mLeftPressed || (Input.mMiddlePressed && mClient->GetGizmoMode() == Editor::EGizmoMode::Translate)};

        return Pressed && mHoveredAxis >= 0 && BeginDrag(mHoveredAxis, &Input, View);
    }

    /**
     * @brief 기즈모 핸들과 드래그 피드백을 그리기 요청에 추가합니다.
     */
    void Editor::FTransformGizmo::Draw(const Engine::FSceneView& View, Engine::FPrimitiveDrawInterface& DrawInterface) {
        BuildHandles(View, &DrawInterface);
        DrawFeedback(&DrawInterface);
    }

    /**
     * @brief 진행 중인 기즈모 드래그를 취소하고 가능한 경우 시작 변환을 복원합니다.
     */
    void Editor::FTransformGizmo::Cancel() {
        EndDrag(true);
    }

    /**
     * @brief 기즈모에서 활성 축을 드래그하고 있는지 검사합니다.
     */
    bool Editor::FTransformGizmo::IsDragging() const {
        return mActiveAxis >= 0;
    }

    /**
     * @brief 마우스가 가리키는 축 인덱스, 없으면 -1을 반환합니다.
     */
    Core::Int32 Editor::FTransformGizmo::GetHoveredAxis() const {
        return mHoveredAxis;
    }

    Engine::USceneComponent* Editor::FTransformGizmo::GetPrimaryTarget() const {
        const Core::TArray<CoreUObject::FObjectHandle> Targets{mContext->GetTransformTargets()};
        return Targets.empty() ? nullptr : CoreUObject::Cast<Engine::USceneComponent>(mContext->ResolveObject(Targets.back()));
    }

    Core::FVector Editor::FTransformGizmo::GetPivot() const {
        if (IsDragging()) {
            return mStartPivot + mTranslation;
        }

        const Engine::USceneComponent* Target{GetPrimaryTarget()};

        return mHasPivot && mPivotRevision == mContext->GetSelectionRevision() ? mPivot : Target != nullptr ? Target->GetTransform().mLocation : Core::FVector{};
    }

    bool Editor::FTransformGizmo::IsPivotDrag() const {
        return IsDragging() && mPivotOnly;
    }

    const Editor::FGizmoSettings* Editor::FTransformGizmo::GetSettings() const {
        return &mSettings;
    }

    bool Editor::FTransformGizmo::CaptureTargets() {
        mTargets.clear();

        for (CoreUObject::FObjectHandle Handle : mContext->GetTransformTargets()) {
            const auto* Component{CoreUObject::Cast<Engine::USceneComponent>(mContext->ResolveObject(Handle))};

            if (Component != nullptr) {
                mTargets.push_back({Handle, Component->GetTransform()});
            }
        }

        return !mTargets.empty();
    }

    void Editor::FTransformGizmo::SetSettings(const Editor::FGizmoSettings& Settings) {
        mSettings = Settings;
    }

    void Editor::FTransformGizmo::EndDrag(bool Restore) {
        if (IsDragging() && Restore && mContext->GetWorldHandle() == mWorldHandle) {
            const bool SameSelection{mContext->GetSelectionRevision() == mSelectionRevision};

            for (const FTargetState& State : mTargets) {
                mContext->ApplyTransform(State.mObject, State.mTransform);
            }

            if (!mDuplicates.empty() && mContext->GetWorld() != nullptr) {
                for (CoreUObject::FObjectHandle Handle : mDuplicates) {
                    mContext->GetWorld()->DestroyActor(Handle);
                }

                if (SameSelection) {
                    mContext->SetSelectedActors(mOriginalSelection);

                    if (mOriginalSelection.size() == 1 && mContext->ResolveObject(mOriginalObject) != nullptr) {
                        mContext->SelectObject(mOriginalObject);
                    }
                }
            }

            if (SameSelection) {
                mHasPivot = mHadPivot;
                mPivot = mOriginalPivot;
                mPivotRevision = mContext->GetSelectionRevision();
            }
        }

        mTargets.clear();
        mDuplicates.clear();
        mActiveAxis = -1;
        mHoveredAxis = -1;
    }

    void Editor::FTransformGizmo::BuildHandles(const Engine::FSceneView& View, Engine::FPrimitiveDrawInterface* DrawInterface) const {
        Core::FMatrix CameraWorld{};

        if (!View.mView.TryInverse(CameraWorld)) {
            return;
        }

        const Core::FVector Right{CameraWorld.GetAxis(0)};
        const Core::FVector Up{CameraWorld.GetAxis(1)};
        const Engine::USceneComponent* Target{GetPrimaryTarget()};
        const Editor::EGizmoMode Mode{mClient->GetGizmoMode()};

        if (DrawInterface == nullptr || Target == nullptr || Target->GetOwner() == nullptr || !mContext->CanEditWorld() || Mode == Editor::EGizmoMode::None) {
            return;
        }

        const Core::FVector Pivot{GetPivot()};
        const float Unit{GetSize(View, Pivot)};

        if (Unit <= 0.0f) {
            return;
        }

        const std::array<Core::FVector, 3> Axes{IsDragging() ? mDragAxes : std::array<Core::FVector, 3>{GetAxis(Target->GetTransform(), 0), GetAxis(Target->GetTransform(), 1), GetAxis(Target->GetTransform(), 2)}};
        const Core::Int32 Highlight{IsDragging() ? mActiveAxis : mHoveredAxis};
        const CoreUObject::FObjectHandle Actor{Target->GetOwner()->GetHandle()};

        const auto Add{[&](Engine::EEditorMesh Mesh, const Core::FMatrix& World, Core::Int32 Handle, Core::Uint32 Priority, const Core::FVector4& Color, bool Pickable = true) {
            DrawInterface->DrawMesh({Mesh, World, Color, Pickable ? Engine::FHitProxy{Engine::EHitProxyType::GizmoAxis, Actor, Handle} : Engine::FHitProxy{}, Priority});
        }};
        const auto Color{[&](Core::Int32 Handle) {
            if (Handle == Highlight) {
                return Core::FVector4{1.0f, 0.85f, 0.05f, 1.0f};
            }

            return Handle < 3 ? Core::FVector4{AxisColors[Handle], 1.0f} : Handle == 3 ? Core::FVector4{0.8f, 0.8f, 0.2f, 0.35f} : Handle == 4 ? Core::FVector4{0.8f, 0.2f, 0.8f, 0.35f} : Handle == 5 ? Core::FVector4{0.2f, 0.8f, 0.8f, 0.35f} : Core::FVector4{0.8f, 0.8f, 0.8f, 1.0f};
        }};

        if (Mode == Editor::EGizmoMode::Rotate) {
            for (Core::Int32 Index{}; Index < 4; ++Index) {
                const Core::Int32 Handle{Index < 3 ? Index : 6};

                if (IsDragging() && Handle != mActiveAxis) {
                    continue;
                }

                const Core::FVector Normal{Index < 3 ? Axes[Index] : View.mForward};
                const float Radius{Index < 3 ? Unit : Unit * 1.2f};
                Core::FVector First{Index < 3 ? (-View.mForward).ProjectedOnPlane(Normal).Normalized() : Right};
                const bool FaceOn{First.IsNearlyZero()};

                if (First.IsNearlyZero()) {
                    First = Perpendicular(Normal);
                }

                const Core::FVector Second{Normal.Cross(First).Normalized()};
                const bool Full{Index == 3 || IsDragging() || FaceOn};
                Add(Full ? Engine::EEditorMesh::Ring : Engine::EEditorMesh::HalfRing, Basis(First * Radius, Second * Radius, Normal * Radius, Pivot), Handle, 2, Color(Handle));

                if (!Full) {
                    Core::FVector4 Back{Color(Handle) * 0.25f};
                    Back.w = 0.45f;
                    Add(Engine::EEditorMesh::HalfRing, Basis(-First * Radius, -Second * Radius, Normal * Radius, Pivot), Handle, 2, Back, false);
                }
            }

            return;
        }

        for (Core::Int32 Index{}; Index < 3; ++Index) {
            if (std::abs(Axes[Index].Dot(View.mForward)) < 0.995f && (!IsDragging() || mActiveAxis == Index)) {
                Add(Mode == Editor::EGizmoMode::Scale ? Engine::EEditorMesh::Hammer : Engine::EEditorMesh::Arrow, Basis(Axes[(Index + 1) % 3] * Unit, Axes[(Index + 2) % 3] * Unit, Axes[Index] * Unit, Pivot), Index, 2, Color(Index));
            }
        }

        const std::array<std::array<Core::Int32, 2>, 3> Pairs{{{0, 1}, {0, 2}, {1, 2}}};

        for (Core::Int32 Index{}; Index < 3; ++Index) {
            const Core::FVector First{Axes[Pairs[Index][0]]};
            const Core::FVector Second{Axes[Pairs[Index][1]]};

            if (std::abs(First.Cross(Second).Dot(View.mForward)) < 0.08f || (IsDragging() && mActiveAxis != Index + 3)) {
                continue;
            }

            const float Near{Mode == Editor::EGizmoMode::Scale ? 18.0f : 20.0f};
            const float Width{Mode == Editor::EGizmoMode::Scale ? 30.0f : 16.0f};
            Add(Mode == Editor::EGizmoMode::Scale ? Engine::EEditorMesh::Triangle : Engine::EEditorMesh::Quad, Basis(First * Width * Unit, Second * Width * Unit, First.Cross(Second) * Unit, Pivot + (First + Second) * Near * Unit), Index + 3, 1, Color(Index + 3));
        }

        const Core::Int32 Center{Mode == Editor::EGizmoMode::Scale ? 7 : 6};

        if (!IsDragging() || mActiveAxis == Center) {
            const Core::FMatrix World{Mode == Editor::EGizmoMode::Scale ? Basis(Axes[0] * 8.0f * Unit, Axes[1] * 8.0f * Unit, Axes[2] * 8.0f * Unit, Pivot) : Basis(Right * 10.0f * Unit, Up * 10.0f * Unit, View.mForward * Unit, Pivot - (Right + Up) * 5.0f * Unit)};
            Add(Mode == Editor::EGizmoMode::Scale ? Engine::EEditorMesh::Cube : Engine::EEditorMesh::Quad, World, Center, 0, Color(Center));
        }
    }

    void Editor::FTransformGizmo::DrawFeedback(Engine::FPrimitiveDrawInterface* DrawInterface) const {
        const Core::FVector Color{1.0f, 0.85f, 0.05f};

        if (!IsDragging()) {
            return;
        }

        if (mDragMode == Editor::EGizmoMode::Rotate) {
            const float Angle{Core::Math::Clamp(mAppliedAngle * 1_rad, -1_turn, 1_turn) / 1_rad};
            const float Radius{mDragSize * 74.0f};
            DrawInterface->DrawLine(mStartPivot, mStartPivot + mStartDirection * Radius, {0.8f, 0.8f, 0.8f});
            DrawInterface->DrawLine(mStartPivot, mStartPivot + Core::FQuat{mPlaneNormal, Angle} * mStartDirection * Radius, Color);

            for (Core::Int32 Index{}; Index < 64; ++Index) {
                DrawInterface->DrawLine(mStartPivot + Core::FQuat{mPlaneNormal, Angle * Index / 64.0f} * mStartDirection * Radius, mStartPivot + Core::FQuat{mPlaneNormal, Angle * (Index + 1) / 64.0f} * mStartDirection * Radius, Color);
            }
        } else if (mDragMode == Editor::EGizmoMode::Translate) {
            DrawInterface->DrawLine(mStartPivot, mStartPivot + mTranslation, Color);
        }
    }

}
