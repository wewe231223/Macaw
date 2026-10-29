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
#include <numbers>
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

#include <algorithm>
#include <cmath>

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 기즈모의 기본 X·Y·Z 단위 축입니다.
     */
    const std::array<FVector, 3> UnitAxes{{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
    /**
     * @brief 기즈모 X·Y·Z 축의 표시 색상입니다.
     */
    const std::array<FVector, 3> AxisColors{{{1.0f, 0.28f, 0.22f}, {0.35f, 0.95f, 0.45f}, {0.32f, 0.58f, 1.0f}}};

    /**
     * @brief 지정한 축과 수직인 안정적인 단위 벡터를 구합니다.
     * @param[in] Axis 회전 또는 편집에 사용할 축입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FVector Perpendicular(const FVector& Axis) {
        return Axis.Cross(std::abs(Axis.z) < 0.8f ? FVector{0.0f, 0.0f, 1.0f} : FVector{0.0f, 1.0f, 0.0f}).Normalized();
    }

    /**
     * @brief 세 기저 축과 원점으로 변환 행렬을 구성합니다.
     * @param[in] X 행렬의 X 기저 축입니다.
     * @param[in] Y 행렬의 Y 기저 축입니다.
     * @param[in] Z 행렬의 Z 기저 축입니다.
     * @param[in] Position 기저 행렬의 월드 원점입니다.
     * @return 기저 축과 평행이동을 반영한 행렬입니다.
     */
    FMatrix Basis(const FVector& X, const FVector& Y, const FVector& Z, const FVector& Position) {
        FMatrix Result{FMatrix::Identity()};
        const std::array<FVector, 4> Rows{X, Y, Z, Position};
        for (Int32 Index{}; Index < 4; ++Index) {
            Result(Index, 0) = Rows[Index].x;
            Result(Index, 1) = Rows[Index].y;
            Result(Index, 2) = Rows[Index].z;
        }
        return Result;
    }

    /**
     * @brief 기즈모 핸들이 주어진 로컬 축에 영향을 주는지 검사합니다.
     * @param[in] Handle 검사할 기즈모 핸들 또는 객체의 식별자입니다.
     * @param[in] Axis 검사할 축의 인덱스입니다.
     * @return 조건을 만족하거나 변경을 적용하면 true입니다.
     */
    bool Affects(Int32 Handle, Int32 Axis) {
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
FTransformGizmo::FTransformGizmo(FEditorContext* Context, const FEditorViewportClient* Client)
	: mContext{Context},
	  mClient{Client} {
}

void FTransformGizmo::SetEditorContext(FEditorContext* Context) {
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
FVector FTransformGizmo::GetAxis(const FTransform& Transform, Int32 Index) const {
    return mClient->GetGizmoSpace() == EGizmoSpace::Local ? Transform.TransformVectorNoScale(UnitAxes[Index]) : UnitAxes[Index];
}

/**
 * @brief 크기를 반환합니다.
 */
float FTransformGizmo::GetSize(const FSceneView& View, const FVector& Pivot) const {
    const float Depth{(Pivot - View.mLocation).Dot(View.mForward)};
    const float Size{std::isfinite(mSettings.mSize) ? std::clamp(mSettings.mSize, 0.5f, 2.0f) : 1.0f};
    if (View.mOrthographic) {
        return 2.0f / (View.mProjection(1, 1) * std::max(View.mSize.y, 1.0f)) * Size;
    }
    return Depth > 0.05f ? 2.0f * Depth * std::tan(Math::ToRadians(View.mFieldOfView) * 0.5f) / std::max(View.mSize.y, 1.0f) * Size : 0.0f;
}

/**
 * @brief 그리는 메시의 CPU 레이 교차 판정으로 선택 액터의 기즈모 HitProxy를 구합니다.
 */
FHitProxy FTransformGizmo::HitTest(const FVector2& Mouse, const FSceneView& View) const {
    if (!mContext->CanEditWorld()) {
        return {};
    }
    FRay Ray{};
    if (!View.Deproject(Mouse, Ray)) {
        return {};
    }
    FPrimitiveDrawInterface Shapes{};
    BuildHandles(View, &Shapes);
    FHitProxy Result{};
    Uint32 Priority{std::numeric_limits<Uint32>::max()};
    float Closest{std::numeric_limits<float>::max()};
    for (const FDrawMesh& Shape : Shapes.GetMeshes()) {
        if (Shape.mHitProxy.mType == EHitProxyType::None || Shape.mPriority > Priority) {
            continue;
        }
        float Distance{Shape.mPriority == Priority ? Closest : std::numeric_limits<float>::max()};
        if (RaycastMesh(&GetEditorMesh(Shape.mMesh), Shape.mWorld, Ray, Distance)) {
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
bool FTransformGizmo::BeginDrag(Int32 Axis, const FViewportInput* Input, const FSceneView& View) {
    USceneComponent* Target{GetPrimaryTarget()};
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
    mPivotOnly = Input->mMiddlePressed && mDragMode == EGizmoMode::Translate;
    mDuplicatePending = Input->mAlt && !mPivotOnly && mDragMode == EGizmoMode::Translate;
    mOriginalSelection = mContext->GetSelectedActors();
    mOriginalObject = mContext->GetSelectedObject() != nullptr ? mContext->GetSelectedObject()->GetHandle() : FObjectHandle{};
    mSelectionRevision = mContext->GetSelectionRevision();
    mTranslation = {};
    mRawAngle = 0.0f;
    mAppliedAngle = 0.0f;
    mDuplicates.clear();
    for (Int32 Index{}; Index < 3; ++Index) {
        mDragAxes[Index] = GetAxis(mStartTransform, Index);
    }
    mAxis = Axis < 3 ? mDragAxes[Axis] : View.mForward;
    mPlaneNormal = View.mForward;
    if (Axis >= 3 && Axis <= 5) {
        mPlaneNormal = Axis == 3 ? mDragAxes[0].Cross(mDragAxes[1]) : Axis == 4 ? mDragAxes[0].Cross(mDragAxes[2]) : mDragAxes[1].Cross(mDragAxes[2]);
        mAxis = {};
        for (Int32 Index{}; Index < 3; ++Index) {
            if (Affects(Axis, Index)) {
                mAxis += mDragAxes[Index];
            }
        }
    } else if (mDragMode == EGizmoMode::Rotate) {
        mPlaneNormal = mAxis;
    } else if (Axis < 3) {
        mPlaneNormal = View.mForward.ProjectedOnPlane(mAxis);
    }
    if (!mPlaneNormal.Normalize()) {
        return false;
    }
    FVector2 Start{};
    FVector2 End{};
    if (!Math::Project(mStartPivot, View.mViewProjection, View.mSize, Start) || !Math::Project(mStartPivot + mAxis * mDragSize * 100.0f, View.mViewProjection, View.mSize, End)) {
        return false;
    }
    mScreenAxis = End - Start;
    const float Length{std::sqrt(mScreenAxis.x * mScreenAxis.x + mScreenAxis.y * mScreenAxis.y)};
    mScreenAxis = Axis >= 6 ? FVector2{0.70710678f, -0.70710678f} : Length > 0.00001f ? mScreenAxis / Length : FVector2{};
    FRay Ray{};
    if (!View.Deproject(Input->mMouse, Ray) || mDragSize <= 0.0f) {
        return false;
    }
    mScreenFallback = std::abs(Ray.mDirection.Dot(mPlaneNormal)) < 0.12f || !Math::IntersectPlane(Ray, mStartPivot, mPlaneNormal, mStartIntersection);
    if (mDragMode == EGizmoMode::Rotate) {
        mStartDirection = mScreenFallback ? Ray.mOrigin + Ray.mDirection * (mStartPivot - Ray.mOrigin).Dot(Ray.mDirection) - mStartPivot : mStartIntersection - mStartPivot;
        mStartDirection = mStartDirection.ProjectedOnPlane(mPlaneNormal).Normalized();
        if (mStartDirection.IsNearlyZero()) {
            return false;
        }
        mPreviousDirection = mStartDirection;
        const FVector Tangent{mPlaneNormal.Cross(mStartDirection)};
        if (Math::Project(mStartPivot + mStartDirection * mDragSize * 74.0f, View.mViewProjection, View.mSize, Start) && Math::Project(mStartPivot + (mStartDirection * 74.0f + Tangent * 10.0f) * mDragSize, View.mViewProjection, View.mSize, End)) {
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
void FTransformGizmo::UpdateDrag(const FViewportInput* Input) {
    FMatrix CameraWorld{};
    if (!mDragView.mView.TryInverse(CameraWorld)) {
        return;
    }
    const FVector Right{CameraWorld.GetAxis(0)};
    const FVector Up{CameraWorld.GetAxis(1)};
    if (Input == nullptr || !IsDragging()) {
        return;
    }
    const FVector2 MouseDelta{Input->mMouse - mStartMouse};
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
    FRay Ray{};
    if (!mDragView.Deproject(Input->mMouse, Ray)) {
        return;
    }
    const float ScreenMovement{MouseDelta.x * mScreenAxis.x + MouseDelta.y * mScreenAxis.y};
    if (mDragMode == EGizmoMode::Rotate) {
        if (mScreenFallback) {
            mRawAngle = ScreenMovement / (74.0f * std::clamp(mSettings.mSize, 0.5f, 2.0f));
        } else {
            FVector Intersection{};
            if (!Math::IntersectPlane(Ray, mStartPivot, mPlaneNormal, Intersection)) {
                return;
            }
            const FVector Direction{(Intersection - mStartPivot).ProjectedOnPlane(mPlaneNormal).Normalized()};
            if (Direction.IsNearlyZero()) {
                return;
            }
            mRawAngle += std::atan2(mPlaneNormal.Dot(mPreviousDirection.Cross(Direction)), std::clamp(mPreviousDirection.Dot(Direction), -1.0f, 1.0f));
            mPreviousDirection = Direction;
        }
        mAppliedAngle = Snap(mRawAngle, Math::ToRadians(mSettings.mRotationStep), mSettings.mRotationSnap);
        const FQuat Rotation{mPlaneNormal, mAppliedAngle};
        for (const FTargetState& State : mTargets) {
            FTransform Desired{State.mTransform};
            Desired.mLocation = mStartPivot + Rotation * (Desired.mLocation - mStartPivot);
            Desired.mRotation = (Rotation * Desired.mRotation).Normalized();
            if (!mContext->ApplyTransform(State.mObject, Desired, true)) {
                Cancel();
                return;
            }
        }
        return;
    }
    if (mDragMode == EGizmoMode::Scale) {
        const float Delta{Snap(ScreenMovement / 100.0f, mSettings.mScaleStep, mSettings.mScaleSnap)};
        const float Uniform{std::max(0.001f, 1.0f + Delta)};
        const auto Primary{mStartTransform.mScale.ToArray()};
        std::array<float, 3> Factors{1.0f, 1.0f, 1.0f};
        for (Int32 Index{}; Index < 3; ++Index) {
            const float Value{Primary[Index] + Delta};
            Factors[Index] = mActiveAxis == 7 ? Uniform : Affects(mActiveAxis, Index) && std::abs(Primary[Index]) > 0.000001f ? (Primary[Index] < 0.0f ? std::min(-0.001f, Value) : std::max(0.001f, Value)) / Primary[Index] : 1.0f;
        }
        for (const FTargetState& State : mTargets) {
            FTransform Desired{State.mTransform};
            auto Scale{Desired.mScale.ToArray()};
            for (Int32 Index{}; Index < 3; ++Index) {
                const float Value{mActiveAxis == 7 ? Scale[Index] * Uniform : Scale[Index] + (Affects(mActiveAxis, Index) ? Delta : 0.0f)};
                Scale[Index] = Scale[Index] < 0.0f ? std::min(-0.001f, Value) : std::max(0.001f, Value);
            }
            Desired.mScale = {Scale[0], Scale[1], Scale[2]};
            const FVector Offset{State.mTransform.mLocation - mStartPivot};
            Desired.mLocation = mStartPivot;
            for (Int32 Index{}; Index < 3; ++Index) {
                Desired.mLocation += mDragAxes[Index] * Offset.Dot(mDragAxes[Index]) * Factors[Index];
            }
            if (!mContext->ApplyTransform(State.mObject, Desired, true)) {
                Cancel();
                return;
            }
        }
        return;
    }
    FVector Delta{};
    FVector Movement{};
    if (mScreenFallback) {
        Movement = mActiveAxis < 3 ? mDragAxes[mActiveAxis] * ScreenMovement * mDragSize : (Right * MouseDelta.x - Up * MouseDelta.y) * mDragSize;
    } else {
        FVector Intersection{};
        if (!Math::IntersectPlane(Ray, mStartPivot, mPlaneNormal, Intersection)) {
            return;
        }
        Movement = Intersection - mStartIntersection;
    }
    for (Int32 Index{}; Index < 3; ++Index) {
        const FVector Axis{mActiveAxis == 6 ? UnitAxes[Index] : mDragAxes[Index]};
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
        FTransform Desired{State.mTransform};
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
bool FTransformGizmo::Input(const FViewportInput& Input, const FSceneView& View, const FHitProxy& Hit) {
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
    const USceneComponent* Target{GetPrimaryTarget()};
    const AActor* Actor{Target != nullptr ? Target->GetOwner() : nullptr};
    mHoveredAxis = Actor != nullptr && mContext->CanEditWorld() && Input.mHovered && !Input.mRightDown && Hit.mType == EHitProxyType::GizmoAxis && Hit.mObject == Actor->GetHandle() && Hit.mAxis >= 0 && Hit.mAxis <= 7 ? Hit.mAxis : -1;
    const bool Pressed{Input.mLeftPressed || (Input.mMiddlePressed && mClient->GetGizmoMode() == EGizmoMode::Translate)};
    return Pressed && mHoveredAxis >= 0 && BeginDrag(mHoveredAxis, &Input, View);
}

/**
 * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
 */
void FTransformGizmo::Draw(const FSceneView& View, FPrimitiveDrawInterface& DrawInterface) {
    BuildHandles(View, &DrawInterface);
    DrawFeedback(&DrawInterface);
}

/**
 * @brief 진행 중인 기즈모 드래그를 취소하고 가능한 경우 시작 변환을 복원합니다.
 */
void FTransformGizmo::Cancel() {
    EndDrag(true);
}

/**
 * @brief 기즈모에서 활성 축을 드래그하고 있는지 검사합니다.
 */
bool FTransformGizmo::IsDragging() const {
    return mActiveAxis >= 0;
}

/**
 * @brief 마우스가 가리키는 축 인덱스, 없으면 -1을 반환합니다.
 */
Int32 FTransformGizmo::GetHoveredAxis() const {
    return mHoveredAxis;
}

USceneComponent* FTransformGizmo::GetPrimaryTarget() const {
    const TArray<FObjectHandle> Targets{mContext->GetTransformTargets()};
    return Targets.empty() ? nullptr : Cast<USceneComponent>(mContext->ResolveObject(Targets.back()));
}

FVector FTransformGizmo::GetPivot() const {
    if (IsDragging()) {
        return mStartPivot + mTranslation;
    }
    const USceneComponent* Target{GetPrimaryTarget()};
    return mHasPivot && mPivotRevision == mContext->GetSelectionRevision() ? mPivot : Target != nullptr ? Target->GetTransform().mLocation : FVector{};
}

bool FTransformGizmo::IsPivotDrag() const {
    return IsDragging() && mPivotOnly;
}

const FGizmoSettings* FTransformGizmo::GetSettings() const {
    return &mSettings;
}

bool FTransformGizmo::CaptureTargets() {
    mTargets.clear();
    for (FObjectHandle Handle : mContext->GetTransformTargets()) {
        const auto* Component{Cast<USceneComponent>(mContext->ResolveObject(Handle))};
        if (Component != nullptr) {
            mTargets.push_back({Handle, Component->GetTransform()});
        }
    }
    return !mTargets.empty();
}

void FTransformGizmo::SetSettings(const FGizmoSettings& Settings) {
    mSettings = Settings;
}

void FTransformGizmo::EndDrag(bool Restore) {
    if (IsDragging() && Restore && mContext->GetWorldHandle() == mWorldHandle) {
        const bool SameSelection{mContext->GetSelectionRevision() == mSelectionRevision};
        for (const FTargetState& State : mTargets) {
            mContext->ApplyTransform(State.mObject, State.mTransform);
        }
        if (!mDuplicates.empty() && mContext->GetWorld() != nullptr) {
            for (FObjectHandle Handle : mDuplicates) {
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

void FTransformGizmo::BuildHandles(const FSceneView& View, FPrimitiveDrawInterface* DrawInterface) const {
    FMatrix CameraWorld{};
    if (!View.mView.TryInverse(CameraWorld)) {
        return;
    }
    const FVector Right{CameraWorld.GetAxis(0)};
    const FVector Up{CameraWorld.GetAxis(1)};
    const USceneComponent* Target{GetPrimaryTarget()};
    const EGizmoMode Mode{mClient->GetGizmoMode()};
    if (DrawInterface == nullptr || Target == nullptr || Target->GetOwner() == nullptr || !mContext->CanEditWorld() || Mode == EGizmoMode::None) {
        return;
    }
    const FVector Pivot{GetPivot()};
    const float Unit{GetSize(View, Pivot)};
    if (Unit <= 0.0f) {
        return;
    }
    const std::array<FVector, 3> Axes{IsDragging() ? mDragAxes : std::array<FVector, 3>{GetAxis(Target->GetTransform(), 0), GetAxis(Target->GetTransform(), 1), GetAxis(Target->GetTransform(), 2)}};
    const Int32 Highlight{IsDragging() ? mActiveAxis : mHoveredAxis};
    const FObjectHandle Actor{Target->GetOwner()->GetHandle()};
    const auto Add{[&](EEditorMesh Mesh, const FMatrix& World, Int32 Handle, Uint32 Priority, const FVector4& Color, bool Pickable = true) {
        DrawInterface->DrawMesh({Mesh, World, Color, Pickable ? FHitProxy{EHitProxyType::GizmoAxis, Actor, Handle} : FHitProxy{}, Priority});
    }};
    const auto Color{[&](Int32 Handle) {
        if (Handle == Highlight) {
            return FVector4{1.0f, 0.85f, 0.05f, 1.0f};
        }
        return Handle < 3 ? FVector4{AxisColors[Handle], 1.0f} : Handle == 3 ? FVector4{0.8f, 0.8f, 0.2f, 0.35f} : Handle == 4 ? FVector4{0.8f, 0.2f, 0.8f, 0.35f} : Handle == 5 ? FVector4{0.2f, 0.8f, 0.8f, 0.35f} : FVector4{0.8f, 0.8f, 0.8f, 1.0f};
    }};
    if (Mode == EGizmoMode::Rotate) {
        for (Int32 Index{}; Index < 4; ++Index) {
            const Int32 Handle{Index < 3 ? Index : 6};
            if (IsDragging() && Handle != mActiveAxis) {
                continue;
            }
            const FVector Normal{Index < 3 ? Axes[Index] : View.mForward};
            const float Radius{Index < 3 ? Unit : Unit * 1.2f};
            FVector First{Index < 3 ? (-View.mForward).ProjectedOnPlane(Normal).Normalized() : Right};
            const bool FaceOn{First.IsNearlyZero()};
            if (First.IsNearlyZero()) {
                First = Perpendicular(Normal);
            }
            const FVector Second{Normal.Cross(First).Normalized()};
            const bool Full{Index == 3 || IsDragging() || FaceOn};
            Add(Full ? EEditorMesh::Ring : EEditorMesh::HalfRing, Basis(First * Radius, Second * Radius, Normal * Radius, Pivot), Handle, 2, Color(Handle));
            if (!Full) {
                FVector4 Back{Color(Handle) * 0.25f};
                Back.w = 0.45f;
                Add(EEditorMesh::HalfRing, Basis(-First * Radius, -Second * Radius, Normal * Radius, Pivot), Handle, 2, Back, false);
            }
        }
        return;
    }
    for (Int32 Index{}; Index < 3; ++Index) {
        if (std::abs(Axes[Index].Dot(View.mForward)) < 0.995f && (!IsDragging() || mActiveAxis == Index)) {
            Add(Mode == EGizmoMode::Scale ? EEditorMesh::Hammer : EEditorMesh::Arrow, Basis(Axes[(Index + 1) % 3] * Unit, Axes[(Index + 2) % 3] * Unit, Axes[Index] * Unit, Pivot), Index, 2, Color(Index));
        }
    }
    const std::array<std::array<Int32, 2>, 3> Pairs{{{0, 1}, {0, 2}, {1, 2}}};
    for (Int32 Index{}; Index < 3; ++Index) {
        const FVector First{Axes[Pairs[Index][0]]};
        const FVector Second{Axes[Pairs[Index][1]]};
        if (std::abs(First.Cross(Second).Dot(View.mForward)) < 0.08f || (IsDragging() && mActiveAxis != Index + 3)) {
            continue;
        }
        const float Near{Mode == EGizmoMode::Scale ? 18.0f : 20.0f};
        const float Width{Mode == EGizmoMode::Scale ? 30.0f : 16.0f};
        Add(Mode == EGizmoMode::Scale ? EEditorMesh::Triangle : EEditorMesh::Quad, Basis(First * Width * Unit, Second * Width * Unit, First.Cross(Second) * Unit, Pivot + (First + Second) * Near * Unit), Index + 3, 1, Color(Index + 3));
    }
    const Int32 Center{Mode == EGizmoMode::Scale ? 7 : 6};
    if (!IsDragging() || mActiveAxis == Center) {
        const FMatrix World{Mode == EGizmoMode::Scale ? Basis(Axes[0] * 8.0f * Unit, Axes[1] * 8.0f * Unit, Axes[2] * 8.0f * Unit, Pivot) : Basis(Right * 10.0f * Unit, Up * 10.0f * Unit, View.mForward * Unit, Pivot - (Right + Up) * 5.0f * Unit)};
        Add(Mode == EGizmoMode::Scale ? EEditorMesh::Cube : EEditorMesh::Quad, World, Center, 0, Color(Center));
    }
}

void FTransformGizmo::DrawFeedback(FPrimitiveDrawInterface* DrawInterface) const {
    const FVector Color{1.0f, 0.85f, 0.05f};
    if (!IsDragging()) {
        return;
    }
    if (mDragMode == EGizmoMode::Rotate) {
        const float Angle{std::clamp(mAppliedAngle, -2.0f * std::numbers::pi_v<float>, 2.0f * std::numbers::pi_v<float>)};
        const float Radius{mDragSize * 74.0f};
        DrawInterface->DrawLine(mStartPivot, mStartPivot + mStartDirection * Radius, {0.8f, 0.8f, 0.8f});
        DrawInterface->DrawLine(mStartPivot, mStartPivot + FQuat{mPlaneNormal, Angle} * mStartDirection * Radius, Color);
        for (Int32 Index{}; Index < 64; ++Index) {
            DrawInterface->DrawLine(mStartPivot + FQuat{mPlaneNormal, Angle * Index / 64.0f} * mStartDirection * Radius, mStartPivot + FQuat{mPlaneNormal, Angle * (Index + 1) / 64.0f} * mStartDirection * Radius, Color);
        }
    } else if (mDragMode == EGizmoMode::Translate) {
        DrawInterface->DrawLine(mStartPivot, mStartPivot + mTranslation, Color);
    }
}
