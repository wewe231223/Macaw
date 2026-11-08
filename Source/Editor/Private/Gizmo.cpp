/**
 * @file Gizmo.cpp
 * @brief 이동, 회전과 스케일 기즈모의 드래그 결과를 선택한 액터에 적용합니다.
 * @ingroup Editor
 */

#include "Editor/Gizmo.h"
#include "Engine/UWorld.h"
#include "Editor/FEditorViewportClient.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/FWorldContext.h"
#include "Engine/AActor.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FTransform.h"
#include "Engine/FSceneView.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/FViewportInput.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/FObjectRegistry.h"
#include "Editor/FEditorContext.h"

#include <algorithm>
#include <cmath>

namespace Editor {

    /**
     * @brief 에디터 편집 경로와 클라이언트가 제공하는 변환 도구 상태를 연결합니다.
     */
    Editor::FTransformGizmo::FTransformGizmo(Editor::FEditorContext* Context, const Editor::FEditorViewportClient* Client)
		: mContext{Context},
		  mClient{Client} {
    }

    /**
     * @brief 선택과 편집 월드에 사용할 컨텍스트를 교체합니다.
     */
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
        return mClient->GetGizmoSpace() == Editor::EGizmoSpace::Local ? Transform.TransformVectorNoScale(Editor::FGizmoCore::GetUnitAxes()[Index]) : Editor::FGizmoCore::GetUnitAxes()[Index];
    }

    /**
     * @brief 현재 피벗, 좌표계와 변환 모드로 기즈모 배치를 구성합니다.
     */
    Editor::FGizmoPlacement Editor::FTransformGizmo::GetPlacement(const Engine::USceneComponent& Target) const {
        Editor::FGizmoPlacement Placement{};
        Placement.mPivot = GetPivot();
        Placement.mMode = mClient->GetGizmoMode();
        Placement.mOwner = Target.GetOwner()->GetHandle();

        for (Core::Int32 Index{}; Index < 3; ++Index) {
            Placement.mAxes[Index] = GetAxis(Target.GetTransform(), Index);
        }

        return Placement;
    }

    /**
     * @brief 그리는 메시의 CPU 레이 교차 판정으로 선택 액터의 기즈모 HitProxy를 구합니다.
     */
    Engine::FHitProxy Editor::FTransformGizmo::HitTest(const Core::FVector2& Mouse, const Engine::FSceneView& View) const {
        const Engine::USceneComponent* Target{GetPrimaryTarget()};

        if (!mContext->CanEditWorld() || Target == nullptr || Target->GetOwner() == nullptr) {
            return {};
        }

        return mCore.HitTest(Mouse, View, GetPlacement(*Target));
    }

    /**
     * @brief 선택 축과 대상의 시작 변환을 저장하고 드래그 평면을 구성합니다.
     */
    bool Editor::FTransformGizmo::BeginDrag(Core::Int32 Axis, const Engine::FViewportInput* Input, const Engine::FSceneView& View) {
        Engine::USceneComponent* Target{GetPrimaryTarget()};

        if (Input == nullptr || Target == nullptr || Target->GetOwner() == nullptr || !mContext->CanEditWorld() || Axis < 0 || Axis > 7) {
            return false;
        }

        if (!CaptureTargets()) {
            return false;
        }

        const Editor::FGizmoPlacement Placement{GetPlacement(*Target)};

        mStartTransform = Target->GetTransform();
        mOriginalPivot = mPivot;
        mHadPivot = mHasPivot && mPivotRevision == mContext->GetSelectionRevision();
        mPivotOnly = Input->mMiddlePressed && Placement.mMode == Editor::EGizmoMode::Translate;
        mDuplicatePending = Input->mAlt && !mPivotOnly && Placement.mMode == Editor::EGizmoMode::Translate;
        mOriginalSelection = mContext->GetSelectedActors();
        mOriginalObject = mContext->GetSelectedObject() != nullptr ? mContext->GetSelectedObject()->GetHandle() : CoreUObject::FObjectHandle{};
        mSelectionRevision = mContext->GetSelectionRevision();
        mDuplicates.clear();

        if (!mCore.BeginDrag(Axis, Input->mMouse, View, Placement)) {
            return false;
        }

        mWorldHandle = mContext->GetWorldHandle();

        return true;
    }

    /**
     * @brief 마우스 이동량을 활성 모드의 변환으로 바꾸어 선택한 액터에 적용합니다.
     */
    void Editor::FTransformGizmo::UpdateDrag(const Engine::FViewportInput* Input) {
        if (Input == nullptr || !IsDragging()) {
            return;
        }

        const Core::FVector2 MouseDelta{Input->mMouse - mCore.GetStartMouse()};

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

        if (!mCore.UpdateDrag(Input->mMouse)) {
            return;
        }

        const Core::FVector& StartPivot{mCore.GetStartPivot()};

        if (mCore.GetDragMode() == Editor::EGizmoMode::Rotate) {
            const Core::FQuat Rotation{mCore.GetRotation()};

            for (const FTargetState& State : mTargets) {
                Core::FTransform Desired{State.mTransform};
                Desired.mLocation = StartPivot + Rotation * (Desired.mLocation - StartPivot);
                Desired.mRotation = (Rotation * Desired.mRotation).Normalized();

                if (!mContext->ApplyTransform(State.mObject, Desired, true)) {
                    Cancel();

                    return;
                }
            }

            return;
        }

        if (mCore.GetDragMode() == Editor::EGizmoMode::Scale) {
            const Core::Int32 ActiveAxis{mCore.GetActiveAxis()};
            const std::array<Core::FVector, 3>& DragAxes{mCore.GetDragAxes()};
            const float Delta{mCore.GetScaleDelta()};
            const float Uniform{std::max(0.001f, 1.0f + Delta)};
            const auto Primary{mStartTransform.mScale.ToArray()};
            std::array<float, 3> Factors{1.0f, 1.0f, 1.0f};

            for (Core::Int32 Index{}; Index < 3; ++Index) {
                const float Value{Primary[Index] + Delta};

                Factors[Index] = ActiveAxis == 7 ? Uniform : Editor::FGizmoCore::Affects(ActiveAxis, Index) && std::abs(Primary[Index]) > 0.000001f ? (Primary[Index] < 0.0f ? std::min(-0.001f, Value) : std::max(0.001f, Value)) / Primary[Index] : 1.0f;
            }

            for (const FTargetState& State : mTargets) {
                Core::FTransform Desired{State.mTransform};
                auto Scale{Desired.mScale.ToArray()};

                for (Core::Int32 Index{}; Index < 3; ++Index) {
                    const float Value{ActiveAxis == 7 ? Scale[Index] * Uniform : Scale[Index] + (Editor::FGizmoCore::Affects(ActiveAxis, Index) ? Delta : 0.0f)};

                    Scale[Index] = Scale[Index] < 0.0f ? std::min(-0.001f, Value) : std::max(0.001f, Value);
                }

                Desired.mScale = {Scale[0], Scale[1], Scale[2]};

                const Core::FVector Offset{State.mTransform.mLocation - StartPivot};

                Desired.mLocation = StartPivot;

                for (Core::Int32 Index{}; Index < 3; ++Index) {
                    Desired.mLocation += DragAxes[Index] * Offset.Dot(DragAxes[Index]) * Factors[Index];
                }

                if (!mContext->ApplyTransform(State.mObject, Desired, true)) {
                    Cancel();

                    return;
                }
            }

            return;
        }

        const Core::FVector& Delta{mCore.GetTranslation()};

        if (mPivotOnly || mHadPivot) {
            mPivot = StartPivot + Delta;
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
            if (mContext->GetWorldHandle() != mWorldHandle || !mContext->CanEditWorld() || mContext->GetSelectionRevision() != mSelectionRevision) {
                Cancel();
            } else if (Input.mEscape) {
                EndDrag(EDragEnd::CancelKeepDuplicates);
            } else if (mPivotOnly ? Input.mMiddleReleased || !Input.mMiddleDown : Input.mLeftReleased || !Input.mLeftDown) {
                if (mPivotOnly ? Input.mMiddleReleased : Input.mLeftReleased) {
                    UpdateDrag(&Input);
                }

                EndDrag(EDragEnd::Commit);
            } else {
                UpdateDrag(&Input);
            }

            return true;
        }

        const Engine::USceneComponent* Target{GetPrimaryTarget()};
        const Engine::AActor* Actor{Target != nullptr ? Target->GetOwner() : nullptr};
        const Core::Int32 Hovered{Actor != nullptr && mContext->CanEditWorld() && Input.mHovered && !Input.mRightDown && Hit.mType == Engine::EHitProxyType::GizmoAxis && Hit.mObject == Actor->GetHandle() && Hit.mAxis >= 0 && Hit.mAxis <= 7 ? Hit.mAxis : -1};

        mCore.SetHoveredAxis(Hovered);

        const bool Pressed{Input.mLeftPressed || (Input.mMiddlePressed && mClient->GetGizmoMode() == Editor::EGizmoMode::Translate)};

        return Pressed && Hovered >= 0 && BeginDrag(Hovered, &Input, View);
    }

    /**
     * @brief 기즈모 핸들과 드래그 피드백을 그리기 요청에 추가합니다.
     */
    void Editor::FTransformGizmo::Draw(const Engine::FSceneView& View, Engine::FPrimitiveDrawInterface& DrawInterface) {
        const Engine::USceneComponent* Target{GetPrimaryTarget()};

        if (Target != nullptr && Target->GetOwner() != nullptr && mContext->CanEditWorld()) {
            mCore.BuildHandles(View, GetPlacement(*Target), DrawInterface);
        }

        mCore.DrawFeedback(DrawInterface);
    }

    /**
     * @brief 진행 중인 기즈모 드래그를 취소하고 가능한 경우 시작 변환을 복원합니다.
     */
    void Editor::FTransformGizmo::Cancel() {
        EndDrag(EDragEnd::Cancel);
    }

    /**
     * @brief 기즈모에서 활성 축을 드래그하고 있는지 검사합니다.
     */
    bool Editor::FTransformGizmo::IsDragging() const {
        return mCore.IsDragging();
    }

    /**
     * @brief 마우스가 가리키는 축 인덱스, 없으면 -1을 반환합니다.
     */
    Core::Int32 Editor::FTransformGizmo::GetHoveredAxis() const {
        return mCore.GetHoveredAxis();
    }

    /**
     * @brief 현재 변환 작업의 기준 장면 컴포넌트를 조회합니다.
     */
    Engine::USceneComponent* Editor::FTransformGizmo::GetPrimaryTarget() const {
        const Core::TArray<CoreUObject::FObjectHandle> Targets{mContext->GetTransformTargets()};

        return Targets.empty() ? nullptr : CoreUObject::Cast<Engine::USceneComponent>(mContext->ResolveObject(Targets.back()));
    }

    /**
     * @brief 선택 상태에 대응하는 현재 월드 피벗을 반환합니다.
     */
    Core::FVector Editor::FTransformGizmo::GetPivot() const {
        if (IsDragging()) {
            return mCore.GetStartPivot() + mCore.GetTranslation();
        }

        const Engine::USceneComponent* Target{GetPrimaryTarget()};

        return mHasPivot && mPivotRevision == mContext->GetSelectionRevision() ? mPivot : Target != nullptr ? Target->GetTransform().mLocation : Core::FVector{};
    }

    /**
     * @brief 현재 드래그가 피벗 위치만 변경하는 작업인지 검사합니다.
     */
    bool Editor::FTransformGizmo::IsPivotDrag() const {
        return IsDragging() && mPivotOnly;
    }

    /**
     * @brief 이 기즈모의 스냅과 크기 설정을 읽기 전용으로 반환합니다.
     */
    const Editor::FGizmoSettings* Editor::FTransformGizmo::GetSettings() const {
        return mCore.GetSettings();
    }

    /**
     * @brief 기존 대상 스냅샷을 비우고 현재 선택된 변환 대상들의 핸들과 변환을 수집합니다.
     */
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

    /**
     * @brief 뷰포트 클라이언트가 검증한 설정의 사본을 저장합니다.
     */
    void Editor::FTransformGizmo::SetSettings(const Editor::FGizmoSettings& Settings) {
        mCore.SetSettings(Settings);
    }

    /**
     * @brief 기즈모 드래그를 확정하거나 원본 변환과 선택으로 되돌립니다.
     */
    void Editor::FTransformGizmo::EndDrag(EDragEnd Result) {
        if (IsDragging() && Result != EDragEnd::Commit && mContext->GetWorldHandle() == mWorldHandle) {
            const bool SameSelection{mContext->GetSelectionRevision() == mSelectionRevision};

            for (const FTargetState& State : mTargets) {
                mContext->ApplyTransform(State.mObject, State.mTransform);
            }

            if (!mDuplicates.empty() && mContext->GetWorld() != nullptr) {
                if (Result == EDragEnd::Cancel) {
                    for (CoreUObject::FObjectHandle Handle : mDuplicates) {
                        mContext->GetWorld()->DestroyActor(Handle);
                    }
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
        mCore.EndDrag();
    }

}
