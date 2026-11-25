/**
 * @file FEditorViewportClient.cpp
 * @brief 에디터 카메라, 선택, 공통 명령과 기즈모 입력을 처리합니다.
 * @ingroup Editor
 */

#include "Core/Types.h"
#include "Editor/FEditorViewportClient.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/Component/UHeightFogComponent.h"
#include "Engine/Component/UWindSourceComponent.h"
#include "Engine/Component/UDecalComponent.h"
#include "Engine/Component/ULightComponentBase.h"
#include "RenderCore/IRendererModule.h"
#include "Core/Math.h"
#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/Component/UBillboardTextComponent.h"
#include "Engine/Component/UBillboardComponent.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/SpriteGeometry.h"
#include "Engine/MeshPicking.h"
#include "Engine/PrimitiveMesh.h"
#include "Engine/Component/UShapeComponent.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "Engine/ULevel.h"
#include "Engine/AActor.h"
#include "Engine/ADecalActor.h"
#include "Engine/ALevelSequenceActor.h"
#include "Engine/AParticleSystemActor.h"
#include "Engine/APawn.h"
#include "Engine/ASkyAtmosphereActor.h"
#include "Engine/AWindSourceActor.h"
#include "Engine/UWorld.h"
#include "Engine/FWorldHit.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FTransform.h"
#include "Core/Math/FRay.h"
#include "Core/Math/FBox.h"
#include "Engine/FSceneView.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/PhysicsBodyDrawing.h"
#include "Engine/FViewportInput.h"
#include "Engine/FSceneViewport.h"
#include "Engine/FWorldHit.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Editor/FEditorContext.h"
#include "Editor/FViewportCommandInfo.h"
#include "Engine/FProject.h"
#include "Asset/FAssetManager.h"
#include "Asset/UStaticMesh.h"
#include "Asset/USkeletalMesh.h"
#include "Asset/UTexture2D.h"
#include "Asset/FAssetFile.h"
#include "Core/Literals.h"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace Core::Literals;

namespace Editor {

    /**
     * @brief FEditorViewportClient의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Editor::FEditorViewportClient::FEditorViewportClient(Editor::FEditorContext* Context)
		: mContext{Context},
		  mCameraSensitivity{Core::Math::ToDegrees(0.2_deg)},
		  mGridSpacing{(1_m).GetBase()},
		  mGizmo{Context, this} {
        ResetCamera();
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Editor::FEditorViewportClient::~FEditorViewportClient() = default;

    /**
     * @brief 기즈모의 진행 중인 변환을 취소하고 공유할 편집 컨텍스트를 변경합니다.
     */
    void Editor::FEditorViewportClient::SetEditorContext(Editor::FEditorContext* Context) {
        if (Context == nullptr || Context == mContext) {
            return;
        }

        mGizmo.SetEditorContext(Context);
        mContext = Context;
        mFocusActive = false;
        mActorIconTextures = {};
    }

    /**
     * @brief 공통 실행 검사를 통과한 명령으로 변환 모드, 좌표계 또는 스냅 설정을 변경합니다.
     */
    bool Editor::FEditorViewportClient::ExecuteCommand(Editor::EViewportCommand Command) {
        if (!CanExecuteCommand(Command)) {
            return false;
        }

        switch (Command) {
        case Editor::EViewportCommand::ToggleGrid:
            mShowFlags.mGrid = !mShowFlags.mGrid;

            return true;
        case Editor::EViewportCommand::Select:
            SetGizmoMode(Editor::EGizmoMode::None);

            break;
        case Editor::EViewportCommand::Translate:
            SetGizmoMode(Editor::EGizmoMode::Translate);

            break;
        case Editor::EViewportCommand::Rotate:
            SetGizmoMode(Editor::EGizmoMode::Rotate);

            break;
        case Editor::EViewportCommand::Scale:
            SetGizmoMode(Editor::EGizmoMode::Scale);

            break;
        case Editor::EViewportCommand::WorldSpace:
            SetGizmoSpace(Editor::EGizmoSpace::World);

            break;
        case Editor::EViewportCommand::LocalSpace:
            SetGizmoSpace(Editor::EGizmoSpace::Local);

            break;
        case Editor::EViewportCommand::ToggleCoordinateSystem:
            SetGizmoSpace(mGizmoSpace == Editor::EGizmoSpace::World ? Editor::EGizmoSpace::Local : Editor::EGizmoSpace::World);

            break;
        case Editor::EViewportCommand::CycleTransformMode:
            SetGizmoMode(mGizmoMode == Editor::EGizmoMode::Translate ? Editor::EGizmoMode::Rotate : mGizmoMode == Editor::EGizmoMode::Rotate ? Editor::EGizmoMode::Scale : Editor::EGizmoMode::Translate);

            break;
        case Editor::EViewportCommand::ToggleTranslationSnap:
        case Editor::EViewportCommand::ToggleRotationSnap:
        case Editor::EViewportCommand::ToggleScaleSnap:
        case Editor::EViewportCommand::ToggleTransformSnap:
            return ToggleGizmoSetting(Command);
        default:
            return false;
        }

        return true;
    }

    /**
     * @brief 드래그 잠금, 스케일 좌표계 제한과 명령 지원 여부를 검사합니다.
     */
    bool Editor::FEditorViewportClient::CanExecuteCommand(Editor::EViewportCommand Command) const {
        if (Command == Editor::EViewportCommand::ToggleGrid) {
            return true;
        }

        if (!CanEditGizmoSettings()) {
            return false;
        }

        switch (Command) {
        case Editor::EViewportCommand::Select:
        case Editor::EViewportCommand::Translate:
        case Editor::EViewportCommand::Rotate:
        case Editor::EViewportCommand::Scale:
        case Editor::EViewportCommand::LocalSpace:
        case Editor::EViewportCommand::CycleTransformMode:
        case Editor::EViewportCommand::ToggleTranslationSnap:
        case Editor::EViewportCommand::ToggleRotationSnap:
        case Editor::EViewportCommand::ToggleScaleSnap:
            return true;
        case Editor::EViewportCommand::ToggleTransformSnap:
            return mGizmoMode != Editor::EGizmoMode::None;
        case Editor::EViewportCommand::WorldSpace:
        case Editor::EViewportCommand::ToggleCoordinateSystem:
            return mGizmoMode != Editor::EGizmoMode::Scale;
        default:
            return false;
        }
    }

    /**
     * @brief 실행 가능 여부와 별개로 현재 변환 모드와 좌표계의 선택 상태를 조회합니다.
     */
    bool Editor::FEditorViewportClient::IsCommandChecked(Editor::EViewportCommand Command) const {
        switch (Command) {
        case Editor::EViewportCommand::ToggleGrid:
            return mShowFlags.mGrid;
        case Editor::EViewportCommand::Select:
            return mGizmoMode == Editor::EGizmoMode::None;
        case Editor::EViewportCommand::Translate:
            return mGizmoMode == Editor::EGizmoMode::Translate;
        case Editor::EViewportCommand::Rotate:
            return mGizmoMode == Editor::EGizmoMode::Rotate;
        case Editor::EViewportCommand::Scale:
            return mGizmoMode == Editor::EGizmoMode::Scale;
        case Editor::EViewportCommand::WorldSpace:
            return mGizmoSpace == Editor::EGizmoSpace::World;
        case Editor::EViewportCommand::LocalSpace:
        case Editor::EViewportCommand::ToggleCoordinateSystem:
            return mGizmoSpace == Editor::EGizmoSpace::Local;
        case Editor::EViewportCommand::ToggleTranslationSnap:
            return mGizmo.GetSettings()->mTranslationSnap;
        case Editor::EViewportCommand::ToggleRotationSnap:
            return mGizmo.GetSettings()->mRotationSnap;
        case Editor::EViewportCommand::ToggleScaleSnap:
            return mGizmo.GetSettings()->mScaleSnap;
        case Editor::EViewportCommand::ToggleTransformSnap:
            return mGizmoMode == Editor::EGizmoMode::Translate ? IsCommandChecked(Editor::EViewportCommand::ToggleTranslationSnap) : mGizmoMode == Editor::EGizmoMode::Rotate ? IsCommandChecked(Editor::EViewportCommand::ToggleRotationSnap) : mGizmoMode == Editor::EGizmoMode::Scale && IsCommandChecked(Editor::EViewportCommand::ToggleScaleSnap);
        default:
            return false;
        }
    }

    /**
     * @brief 현재 프레임의 뷰포트 입력을 처리합니다.
     */
    void Editor::FEditorViewportClient::Input(Engine::FSceneViewport* Viewport, const Engine::FViewportInput& Input, float DeltaTime) {
        if (Viewport == nullptr || mContext == nullptr) {
            return;
        }

        const FVector2 Size{Viewport->GetSize()};
        const bool CameraInput{(Input.mHovered || Input.mFocused) && (Input.mRightDown || Input.mMiddleDown || (Input.mAlt && Input.mLeftDown) || (Input.mHovered && Input.mWheel != 0.0f) || (Input.mFocused && (Input.mForward || Input.mBackward || Input.mLeft || Input.mRight || Input.mUp || Input.mDown)))};
        const bool PreviewingCamera{GetPreviewCamera() != nullptr};

        if (CameraInput || mGizmo.IsDragging() || PreviewingCamera) {
            mFocusActive = false;
        }

        UpdateCameraFocus(DeltaTime);

        if (mViewMode == Engine::EViewMode::PhysicsBodies) {
            if (!PreviewingCamera) {
                NavigateCamera(Input, Size, DeltaTime);
            }

            return;
        }

        if (!mGizmo.IsDragging() && Input.mFocused && !Input.mRightDown && !Input.mAlt && !Input.mControl) {
            bool ModeRequested{};

            for (const Editor::FViewportCommandInfo& Info : Editor::FViewportCommandInfo::GetCommands()) {
                if (Info.mInput == nullptr || !(Input.*Info.mInput)) {
                    continue;
                }

                const bool ModeCommand{Info.mTransformMode || Info.mCommand == Editor::EViewportCommand::CycleTransformMode};

                if ((ModeCommand && ModeRequested) || (Info.mCommand == Editor::EViewportCommand::CycleTransformMode && Input.mLeftDown)) {
                    continue;
                }

                ModeRequested = ModeRequested || ModeCommand;

                ExecuteCommand(Info.mCommand);
            }
        }

        const Engine::FSceneView View{CalcSceneView(Size)};
        const Engine::FHitProxy Hit{!mGizmo.IsDragging() && Input.mHovered ? Viewport->GetHitProxy(Input.mMouse) : Engine::FHitProxy{}};

        if (mGizmo.Input(Input, View, Hit)) {
            return;
        }

        if (!PreviewingCamera && NavigateCamera(Input, Size, DeltaTime)) {
            return;
        }

        if (Input.mDelete && Input.mFocused) {
            mContext->DestroySelection();

            return;
        }

        if (Input.mLeftPressed && Input.mHovered && !Input.mAlt && mContext->GetWorld() != nullptr) {
            if (Hit.mType == Engine::EHitProxyType::Object) {
                CoreUObject::UObject* Object{mContext->ResolveObject(Hit.mObject)};
                const auto* Component{CoreUObject::Cast<Engine::UActorComponent>(Object)};
                Engine::AActor* Actor{Component != nullptr ? Component->GetOwner() : CoreUObject::Cast<Engine::AActor>(Object)};

                if (Actor != nullptr) {
                    if (Input.mControl) {
                        mContext->ToggleActorSelection(CoreUObject::MakeObjectRef(Actor));
                    } else {
                        mContext->SelectActor(CoreUObject::MakeObjectRef(Actor));
                    }
                }
            } else if (Hit.mType == Engine::EHitProxyType::None && !Input.mControl) {
                mContext->ClearSelection();
            }
        }
    }

    /**
     * @brief 뷰포트 입력으로 카메라 이동, 회전, 팬 또는 오빗을 적용합니다.
     * @note 자유 시점 조작은 피벗까지의 거리를 유지하며 갱신한 카메라의 시선 앞에 피벗을 배치합니다.
     */
    bool Editor::FEditorViewportClient::NavigateCamera(const Engine::FViewportInput& Input, const FVector2& Size, float DeltaTime) {
        if (!Input.mHovered && !Input.mFocused) {
            return false;
        }

        if (Input.mFocusSelection && Input.mFocused && !Input.mRightDown && !Input.mAlt) {
            FocusActor(mContext->GetSelectedActor(), Size);
        }

        const FQuat Rotation{mCameraRotation.Quaternion()};

        if (mViewType != Editor::EEditorViewportType::Perspective) {
            if (Input.mWheel != 0.0f && Input.mHovered) {
                const float PreviousWidth{mOrthographicWidth};

                mOrthographicWidth = std::clamp(mOrthographicWidth * std::pow(0.85f, Input.mWheel), (0.1_m).GetBase(), (10000_m).GetBase());

                const float PixelScale{(PreviousWidth - mOrthographicWidth) / std::max(Size.x, 1.0f)};
                const FVector Offset{Rotation.RightVector() * ((Input.mMouse.x - Size.x * 0.5f) * PixelScale) + Rotation.UpVector() * ((Size.y * 0.5f - Input.mMouse.y) * PixelScale)};

                mCameraLocation += Offset;
                mOrbitPivot += Offset;
            }

            if (Input.mRightDown || Input.mMiddleDown) {
                const float PixelScale{mOrthographicWidth / std::max(Size.x, 1.0f)};
                const FVector Offset{(Rotation.RightVector() * -Input.mDelta.x + Rotation.UpVector() * Input.mDelta.y) * PixelScale};

                mCameraLocation += Offset;
                mOrbitPivot += Offset;

                return true;
            }

            return Input.mAlt || (Input.mWheel != 0.0f && Input.mHovered);
        }

        if (Input.mWheel != 0.0f && Input.mHovered) {
            if (Input.mRightDown && !Input.mAlt) {
                SetCameraSpeed(std::clamp(mCameraSpeed * std::pow(1.25f, Input.mWheel), 0.1_mps, 128_mps));
            } else {
                mCameraLocation += Rotation.ForwardVector() * (mCameraSpeed * 0.5_s * Input.mWheel).GetBase();
            }
        }

        if (Input.mMiddleDown || (Input.mLeftDown && Input.mRightDown && !Input.mAlt)) {
            const float Distance{std::max(mCameraLocation.Distance(mOrbitPivot), (1_m).GetBase())};
            const float PixelScale{2.0f * Distance * Core::Math::Tan(30_deg) / std::max(Size.y, 1.0f)};
            const FVector Pan{Rotation * FVector{0.0f, -Input.mDelta.x, Input.mDelta.y} * PixelScale};

            mCameraLocation += Pan;
            mOrbitPivot += Pan;

            return true;
        }

        if (Input.mAlt && (Input.mLeftDown || Input.mRightDown)) {
            float Distance{std::max(mCameraLocation.Distance(mOrbitPivot), (0.1_m).GetBase())};
            FVector Forward{mOrbitPivot - mCameraLocation};

            if (Forward.Normalize()) {
                const FRotator Facing{Forward.Rotation()};

                mCameraRotation.yaw = Facing.yaw;
                mCameraRotation.pitch = std::clamp(Facing.pitch, Core::Math::ToDegrees(-89_deg), Core::Math::ToDegrees(89_deg));
            }

            if (Input.mLeftDown) {
                mCameraRotation.yaw += Input.mDelta.x * mCameraSensitivity;
                mCameraRotation.pitch = std::clamp(mCameraRotation.pitch - Input.mDelta.y * mCameraSensitivity, Core::Math::ToDegrees(-89_deg), Core::Math::ToDegrees(89_deg));
            } else {
                Distance = std::clamp(Distance * std::exp((Input.mDelta.y - Input.mDelta.x) * 0.01f), (0.1_m).GetBase(), (100000_m).GetBase());
            }

            mCameraLocation = mOrbitPivot - mCameraRotation.Vector() * Distance;

            return true;
        }

        const FVector LocalMovement{static_cast<float>(Input.mForward) - static_cast<float>(Input.mBackward), static_cast<float>(Input.mRight) - static_cast<float>(Input.mLeft), 0.0f};
        const float VerticalMovement{static_cast<float>(Input.mUp) - static_cast<float>(Input.mDown)};
        const bool Moving{Input.mFocused && (LocalMovement.LengthSquared() > 0.0f || VerticalMovement != 0.0f)};

        if (Input.mRightDown || Moving) {
            const float OrbitDistance{std::max(mCameraLocation.Distance(mOrbitPivot), (0.1_m).GetBase())};

            if (Input.mRightDown) {
                mCameraRotation.yaw += Input.mDelta.x * mCameraSensitivity;
                mCameraRotation.pitch = std::clamp(mCameraRotation.pitch - Input.mDelta.y * mCameraSensitivity, Core::Math::ToDegrees(-89_deg), Core::Math::ToDegrees(89_deg));
            }

            const FQuat FlightRotation{mCameraRotation.Quaternion()};
            FVector Movement{FlightRotation * LocalMovement};
            Movement.z += VerticalMovement;

            mCameraLocation += Movement.Normalized() * (mCameraSpeed * std::max(Core::FTime::FromBase(DeltaTime), 0_s) * (Input.mFast ? 3.0f : 1.0f)).GetBase();
            mOrbitPivot = mCameraLocation + FlightRotation.ForwardVector() * OrbitDistance;
        }

        return Input.mRightDown || Input.mAlt || (Input.mWheel != 0.0f && Input.mHovered);
    }

    /**
     * @brief 선택한 액터가 화면에 들어오도록 카메라 위치와 중심을 조정합니다.
     */
    void Editor::FEditorViewportClient::FocusActor(const Engine::AActor* Actor, const FVector2& Size) {
        if (Actor == nullptr || Actor->IsPendingDestroy() || mGizmo.IsDragging() || GetPreviewCamera() != nullptr) {
            return;
        }

        const Engine::FSceneView View{CalcSceneView(Size)};
        const FVector Location{Actor->GetRootComponent() != nullptr ? Actor->GetRootComponent()->GetTransform().mLocation : FVector{}};
        FBox Bounds{Location - FVector{(0.5_m).GetBase(), (0.5_m).GetBase(), (0.5_m).GetBase()}, Location + FVector{(0.5_m).GetBase(), (0.5_m).GetBase(), (0.5_m).GetBase()}};
        bool HasBounds{};

        for (const auto& Component : Actor->GetComponents()) {
            const auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(Component.get())};

            if (Primitive == nullptr) {
                continue;
            }

            const FBox ComponentBounds{CalcComponentBounds(Primitive, View)};

            if (!ComponentBounds.mMinimum.IsFinite() || !ComponentBounds.mMaximum.IsFinite() || ComponentBounds.mMinimum.x > ComponentBounds.mMaximum.x || ComponentBounds.mMinimum.y > ComponentBounds.mMaximum.y || ComponentBounds.mMinimum.z > ComponentBounds.mMaximum.z) {
                continue;
            }

            Bounds = HasBounds ? FBox{Bounds.mMinimum.Min(ComponentBounds.mMinimum), Bounds.mMaximum.Max(ComponentBounds.mMaximum)} : ComponentBounds;
            HasBounds = true;
        }

        if (!Bounds.mMinimum.IsFinite() || !Bounds.mMaximum.IsFinite()) {
            return;
        }

        const float Radius{std::max(Bounds.mMinimum.Distance(Bounds.mMaximum) * 0.5f, (0.25_m).GetBase())};
        const float Aspect{std::max(Size.x, 1.0f) / std::max(Size.y, 1.0f)};

        mFocusStartLocation = mCameraLocation;
        mFocusStartPivot = mOrbitPivot;
        mFocusStartWidth = mOrthographicWidth;
        mFocusTargetPivot = Bounds.mMinimum.Lerp(Bounds.mMaximum, 0.5f);
        mFocusTargetWidth = mOrthographicWidth;

        if (mViewType != Editor::EEditorViewportType::Perspective) {
            mFocusTargetWidth = std::clamp(Radius * 2.4f * std::max(Aspect, 1.0f), (0.1_m).GetBase(), (10000_m).GetBase());
            mFocusTargetLocation = mFocusTargetPivot - mCameraRotation.Vector() * (5000_m).GetBase();
        } else {
            const Core::FAngle HalfFieldOfView{Core::Math::Atan2(Core::Math::Tan(mFieldOfView * 0.5f) * std::min(Aspect, 1.0f), 1.0f)};
            const float Distance{std::max(Radius / Core::Math::Sin(HalfFieldOfView) * 1.2f, Radius + mNearClip.GetBase() * 1.2f)};

            mFocusTargetLocation = mFocusTargetPivot - mCameraRotation.Vector() * Distance;
        }

        mFocusElapsed = 0.0f;
        mFocusActive = true;
    }

    void Editor::FEditorViewportClient::UpdateCameraFocus(float DeltaTime) {
        if (!mFocusActive || !std::isfinite(DeltaTime) || DeltaTime <= 0.0f) {
            return;
        }

        constexpr float Duration{(0.35_s).GetBase()};

        mFocusElapsed = std::min(mFocusElapsed + DeltaTime, Duration);

        const float Progress{mFocusElapsed / Duration};
        const float Alpha{Progress * Progress * (3.0f - 2.0f * Progress)};

        mCameraLocation = mFocusStartLocation.Lerp(mFocusTargetLocation, Alpha);
        mOrbitPivot = mFocusStartPivot.Lerp(mFocusTargetPivot, Alpha);
        mOrthographicWidth = mFocusStartWidth + (mFocusTargetWidth - mFocusStartWidth) * Alpha;
        mFocusActive = mFocusElapsed < Duration;
    }

    /**
     * @brief 월드 원점에 고정한 그리드와 에디터 보조 요소를 포함한 뷰포트를 그립니다.
     */
    void Editor::FEditorViewportClient::SyncFieldOfViewWithActiveCamera() {
        const Engine::UWorld* World{mContext != nullptr ? mContext->GetWorld() : nullptr};
        const Engine::UCameraComponent* Camera{World != nullptr ? World->GetActiveCamera() : nullptr};

        if (Camera == nullptr) {
            mObservedCamera = nullptr;

            return;
        }

        // 렌즈 모드에서는 초점거리·센서 높이로 계산한 시야각이므로 렌즈 값 변경도 함께 따라갑니다.
        const Core::FAngle FieldOfView{Camera->GetFieldOfView()};
        const bool SameCamera{mObservedCamera == Camera};
        const bool Changed{SameCamera && FieldOfView != mObservedCameraFieldOfView};

        mObservedCamera = Camera;
        mObservedCameraFieldOfView = FieldOfView;

        if (Changed && FieldOfView.IsFinite()) {
            mFieldOfView = std::clamp(FieldOfView, 1_deg, 179_deg);
        }
    }

    void Editor::FEditorViewportClient::Draw(Engine::FSceneViewport* Viewport) {
        Engine::UWorld* World{mContext->GetWorld()};

        if (World == nullptr || World->GetScene() == nullptr || mContext->GetRenderer() == nullptr) {
            return;
        }

        SyncFieldOfViewWithActiveCamera();

        const Engine::FSceneView View{CalcSceneView(Viewport->GetSize())};

        mDrawInterface.Clear();

        if (mViewMode == Engine::EViewMode::PhysicsBodies) {
            Engine::PhysicsBodyDrawing::DrawWorldBodies(mDrawInterface, *World, mLineThickness);
            const Engine::FSceneViewFamily Family{World->GetScene(), Viewport->GetRenderTarget(), View, &mDrawInterface, mViewMode};

            mContext->GetRenderer()->BeginRenderingViewFamily(Family);

            return;
        }

        if (mShowFlags.mAxes) {
            constexpr float AxisLength{(1000_m).GetBase()};

            mDrawInterface.DrawLine({-AxisLength, 0.0f, 0.0f}, {AxisLength, 0.0f, 0.0f}, {1.0f, 0.25f, 0.2f}, true, mLineThickness);
            mDrawInterface.DrawLine({0.0f, -AxisLength, 0.0f}, {0.0f, AxisLength, 0.0f}, {0.3f, 0.9f, 0.4f}, true, mLineThickness);
            mDrawInterface.DrawLine({0.0f, 0.0f, -AxisLength}, {0.0f, 0.0f, AxisLength}, {0.3f, 0.55f, 1.0f}, true, mLineThickness);
        }

        mContext->DrawClothGridTest(mDrawInterface, mLineThickness);
        mContext->DrawClothAssetTest(mDrawInterface, mLineThickness);
        mContext->DrawClothSimTest(mDrawInterface, mLineThickness);

        // 켜져 있으면 월드의 스켈레탈 메시 컴포넌트마다 마지막 천 결과를 그립니다.
        if (mContext->IsClothDebugDrawEnabled() && World->GetPersistentLevel() != nullptr) {
            for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
                for (const auto& Component : Actor->GetComponents()) {
                    if (const auto* SkeletalMeshComponent{CoreUObject::Cast<Engine::USkeletalMeshComponent>(Component.get())}) {
                        SkeletalMeshComponent->DebugDrawClothing(mDrawInterface, mLineThickness);
                    }
                }
            }
        }

        if (World->GetPersistentLevel() != nullptr) {
            const Core::TArray<CoreUObject::TObjectRef<Engine::AActor>> SelectedActors{mContext->GetSelectedActors()};
            const auto* SelectedComponent{CoreUObject::Cast<Engine::UActorComponent>(mContext->GetSelectedObject())};

            for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
                if (!Actor->IsVisible()) {
                    continue;
                }

                Engine::FPrimitiveSceneProxy Icon{};

                if (TryCreateActorIcon(*Actor, Icon)) {
                    mDrawInterface.DrawSprite(Icon);
                }

                const bool Highlight{std::find(SelectedActors.begin(), SelectedActors.end(), CoreUObject::MakeObjectRef(Actor)) != SelectedActors.end()};

                if (Highlight && Actor->GetRootComponent() != nullptr) {
                    const auto* SelectedSceneComponent{SelectedComponent != nullptr && SelectedComponent->GetOwner() == Actor.get() ? CoreUObject::Cast<Engine::USceneComponent>(SelectedComponent) : nullptr};
                    FVector Minimum{SelectedSceneComponent != nullptr ? SelectedSceneComponent->GetTransform().mLocation : Actor->GetRootComponent()->GetTransform().mLocation};
                    FVector Maximum{Minimum};
                    bool HasBounds{};

                    for (const auto& Component : Actor->GetComponents()) {
                        auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(Component.get())};

                        if (Primitive == nullptr || (SelectedSceneComponent != nullptr && Primitive != SelectedSceneComponent)) {
                            continue;
                        }

                        const FBox Bounds{CalcComponentBounds(Primitive, View)};

                        if (!HasBounds) {
                            Minimum = Bounds.mMinimum;
                            Maximum = Bounds.mMaximum;
                            HasBounds = true;
                        } else {
                            Minimum = {std::min(Minimum.x, Bounds.mMinimum.x), std::min(Minimum.y, Bounds.mMinimum.y), std::min(Minimum.z, Bounds.mMinimum.z)};
                            Maximum = {std::max(Maximum.x, Bounds.mMaximum.x), std::max(Maximum.y, Bounds.mMaximum.y), std::max(Maximum.z, Bounds.mMaximum.z)};
                        }
                    }

                    const FVector ScreenUp{View.mView(0, 1), View.mView(1, 1), View.mView(2, 1)};
                    const FVector Extent{(Maximum - Minimum) * 0.5f};
                    const float Height{std::abs(ScreenUp.x) * Extent.x + std::abs(ScreenUp.y) * Extent.y + std::abs(ScreenUp.z) * Extent.z};
                    const FVector Center{(Minimum + Maximum) * 0.5f};
                    const FVector4 Clip{FVector4{Center, 1.0f} * View.mViewProjection};
                    const float Margin{Clip.w > 0.0f ? 32.0f * Clip.w / (std::max(View.mSize.y, 1.0f) * View.mProjection(1, 1)) : 0.0f};
                    const Core::FString& Name{SelectedComponent != nullptr && SelectedComponent->GetOwner() == Actor.get() ? SelectedComponent->GetName() : Actor->GetName()};

                    mDrawInterface.DrawString({Center + ScreenUp * (Height + Margin), Name, {1.0f, 0.9f, 0.55f, 1.0f}, 18.0f, {1.0f, 1.0f}, true, false});
                }

                for (const auto& Component : Actor->GetComponents()) {
                    auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(Component.get())};

                    if (Primitive != nullptr && Primitive->ShouldCreateRenderState() && (mShowFlags.mBounds || Highlight)) {
                        const bool DepthTest{!Highlight || CoreUObject::Cast<Engine::UBillboardComponent>(Primitive) != nullptr || CoreUObject::Cast<Engine::UBillboardTextComponent>(Primitive) != nullptr};

                        mDrawInterface.DrawBox(CalcComponentBounds(Primitive, View), Highlight ? FVector{1.0f, 0.65f, 0.15f} : FVector{0.25f, 0.8f, 0.95f}, DepthTest, mLineThickness);
                    }

                    const auto* Shape{CoreUObject::Cast<Engine::UShapeComponent>(Component.get())};

                    if (Shape != nullptr) {
                        if (mShowFlags.mCollision) {
                            Shape->DrawCollision(mDrawInterface, Shape->GetShapeColor(), mLineThickness);
                        } else if (!Shape->ShouldDrawOnlyIfSelected() || Highlight) {
                            Shape->DrawDebug(mDrawInterface);
                        }
                    } else if (Primitive != nullptr && mShowFlags.mCollision) {
                        Primitive->DrawCollision(mDrawInterface, Highlight, mLineThickness);
                    }

                    if (auto* Light{CoreUObject::Cast<Engine::ULightComponentBase>(Component.get())}; Light != nullptr && mShowFlags.mLightVisualizers) {
                        Light->DrawVisualization(mDrawInterface, Highlight);
                    }

                    if (const auto* Wind{CoreUObject::Cast<Engine::UWindSourceComponent>(Component.get())}; Wind != nullptr) {
                        Wind->DrawVisualization(mDrawInterface, Highlight);
                    }

                    if (auto* Fog{CoreUObject::Cast<Engine::UHeightFogComponent>(Component.get())}; Fog != nullptr && Highlight) {
                        const FVector Location{Fog->GetTransform().mLocation};

                        mDrawInterface.DrawBox(FBox{Location - FVector{(0.5_m).GetBase(), (0.5_m).GetBase(), (0.5_m).GetBase()}, Location + FVector{(0.5_m).GetBase(), (0.5_m).GetBase(), (0.5_m).GetBase()}}, {1.0f, 0.65f, 0.15f}, true, mLineThickness);
                    }

                    if (const auto* Decal{CoreUObject::Cast<Engine::UDecalComponent>(Component.get())}; Decal != nullptr && Highlight) {
                        const FMatrix DecalToWorld{Decal->GetDecalToWorld()};
                        const FVector Color{1.0f, 0.65f, 0.15f};
                        Core::TStaticArray<FVector, 8> Corners{};

                        for (Index_t Corner{}; Corner < Corners.size(); ++Corner) {
                            Corners[Corner] = DecalToWorld.TransformPosition({(Corner & 1u) != 0 ? 1.0f : -1.0f, (Corner & 2u) != 0 ? 1.0f : -1.0f, (Corner & 4u) != 0 ? 1.0f : -1.0f});
                        }

                        for (Index_t Corner{}; Corner < Corners.size(); ++Corner) {
                            for (Index_t Axis{1}; Axis < Corners.size(); Axis <<= 1) {
                                if ((Corner & Axis) == 0) {
                                    mDrawInterface.DrawLine(Corners[Corner], Corners[Corner | Axis], Color, true, mLineThickness);
                                }
                            }
                        }
                    }
                }
            }
        }

        mGizmo.Draw(View, mDrawInterface);

        const Engine::FSceneViewFamily Family{World->GetScene(), Viewport->GetRenderTarget(), View, &mDrawInterface, mViewMode, mShowFlags, mShowFlags.mGrid ? mGridSpacing : 0.0f};

        mContext->GetRenderer()->BeginRenderingViewFamily(Family);
    }

    bool Editor::FEditorViewportClient::TryCreateActorIcon(const Engine::AActor& Actor, Engine::FPrimitiveSceneProxy& Proxy) const {
        const Engine::USceneComponent* Root{Actor.GetRootComponent()};

        if (!mShowFlags.mSprites || Actor.IsPendingDestroy() || Root == nullptr) {
            return false;
        }

        EActorIcon Icon{};

        if (Actor.IsA<Engine::ADecalActor>()) {
            Icon = EActorIcon::Decal;
        } else if (Actor.IsA<Engine::ALevelSequenceActor>()) {
            Icon = EActorIcon::LevelSequence;
        } else if (Actor.IsA<Engine::AParticleSystemActor>()) {
            Icon = EActorIcon::ParticleSystem;
        } else if (Actor.IsA<Engine::APawn>()) {
            Icon = EActorIcon::Pawn;
        } else if (Actor.IsA<Engine::ASkyAtmosphereActor>()) {
            Icon = EActorIcon::SkyAtmosphere;
        } else if (Actor.IsA<Engine::AWindSourceActor>()) {
            Icon = EActorIcon::Wind;
        } else if (Actor.GetClass() == Engine::AActor::StaticClass() && Root->GetClass() == Engine::USceneComponent::StaticClass()) {
            for (const auto& Component : Actor.GetComponents()) {
                if (Component->IsA<Engine::UPrimitiveComponent>()) {
                    return false;
                }
            }

            Icon = EActorIcon::EmptyActor;
        } else {
            return false;
        }

        static constexpr Core::TStaticArray<const char*, static_cast<Index_t>(EActorIcon::Count)> Paths{"/Engine/Editor/Icon/EmptyActor.png", "/Engine/Editor/Icon/S_DecalActorIcon.png", "/Engine/Editor/Icon/S_LevelSequence.png", "/Engine/Editor/Icon/S_Emitter.png", "/Engine/Editor/Icon/S_Pawn.png", "/Engine/Editor/Icon/S_SkyAtmosphere.png", "/Engine/Editor/Icon/S_Wind.png"};
        const Index_t Index{static_cast<Index_t>(Icon)};
        CoreUObject::TObjectRef<Asset::UTexture2D>& Texture{mActorIconTextures[Index]};

        if (Texture == nullptr) {
            const Engine::UWorld* World{Actor.GetWorld()};
            Asset::FAssetManager* Assets{World != nullptr ? World->GetAssets() : nullptr};

            if (Assets == nullptr) {
                return false;
            }

            Core::FString Error{};
            Texture = Assets->LoadTexture(Paths[Index], Error);

            if (Texture == nullptr) {
                return false;
            }
        }

        Proxy.mId = Actor.GetGuid();
        Proxy.mWorld = FTransform{Root->GetTransform().mLocation, {}, {1.0f, 1.0f, 1.0f}}.ToMatrix();
        Proxy.mSprite = Engine::FDrawSprite{Texture, {1.0f, 1.0f}};

        return true;
    }

    /**
     * @brief 현재 카메라와 뷰포트 크기로 뷰·투영 행렬을 계산합니다.
     */
    Engine::FSceneView Editor::FEditorViewportClient::CalcSceneView(const FVector2& Size) const {
        if (const Engine::UCameraComponent* PreviewCamera{GetPreviewCamera()}) {
            Engine::FSceneView PreviewView{};

            if (PreviewCamera->GetCameraView(Size, PreviewView)) {
                return PreviewView;
            }
        }

        const FQuat Rotation{mCameraRotation.Quaternion()};

        Engine::FSceneView View{};
        View.mLocation = mCameraLocation;
        View.mForward = Rotation.ForwardVector();
        View.mSize = Size;
        View.mOrthographic = mViewType != Editor::EEditorViewportType::Perspective;
        View.mOrthographicWidth = mOrthographicWidth;
        View.mFieldOfView = mFieldOfView;

        View.mView = FMatrix::LookTo(mCameraLocation, View.mForward, Rotation.UpVector());

        const float Aspect{std::max(Size.x, 1.0f) / std::max(Size.y, 1.0f)};

        View.mProjection = View.mOrthographic ? FMatrix::Orthographic(mOrthographicWidth, mOrthographicWidth / Aspect, (0.05_m).GetBase(), (10000_m).GetBase()) : FMatrix::Perspective(mFieldOfView, Aspect, mNearClip, mFarClip);
        View.mViewProjection = View.mView * View.mProjection;

        const Engine::UWorld* World{mContext != nullptr ? mContext->GetWorld() : nullptr};

        // 편집 카메라의 시점을 유지하면서 활성 카메라의 피사계 심도 설정과 렌즈를 적용합니다.
        // 편집 카메라의 시야각은 카메라 렌즈와 무관하므로 흐림 크기는 카메라의 초점거리·센서로 계산합니다.
        if (const Engine::UCameraComponent* Camera{World != nullptr ? World->GetActiveCamera() : nullptr}) {
            View.mDepthOfFieldSettings = Camera->GetDepthOfFieldSettings();
            View.mSensorHeight = Camera->GetSensorHeight();
            View.mLensFocalLength = Camera->GetFocalLength();
        }

        return View;
    }

    const Engine::UCameraComponent* Editor::FEditorViewportClient::GetPreviewCamera() const {
        return mViewType == Editor::EEditorViewportType::Perspective && mContext != nullptr ? mContext->GetPreviewCamera() : nullptr;
    }

    /**
     * @brief 에디터 카메라를 초기 위치와 회전으로 되돌립니다.
     */
    void Editor::FEditorViewportClient::ResetCamera() {
        mFocusActive = false;
        const Engine::FEditorCameraState DefaultCamera{};
        mCameraLocation = DefaultCamera.GetLocation();
        mCameraRotation = DefaultCamera.GetRotation();

        mOrbitPivot = {};
        mCameraSpeed = 4_mps;
        mPerspectiveLocation = mCameraLocation;
        mPerspectiveRotation = mCameraRotation;
        mPerspectivePivot = {};
        mOrthographicWidth = (20_m).GetBase();
        mFieldOfView = DefaultCamera.GetFieldOfView();
        mNearClip = DefaultCamera.GetNearClip();
        mFarClip = DefaultCamera.GetFarClip();

        if (mViewType != Editor::EEditorViewportType::Perspective) {
            const Editor::EEditorViewportType Type{mViewType};

            mViewType = Editor::EEditorViewportType::Perspective;

            SetViewType(Type);
        }
    }

    /**
     * @brief 유효한 위치와 회전으로 에디터 카메라를 설정합니다.
     */
    void Editor::FEditorViewportClient::SetCamera(const FVector& Location, const FRotator& Rotation) {
        mFocusActive = false;
        SetViewType(Editor::EEditorViewportType::Perspective);

        mCameraLocation = Location;
        mCameraRotation = Rotation;
        mOrbitPivot = Location + Rotation.Vector() * (10_m).GetBase();
    }

    /**
     * @brief 원근 또는 정사영 뷰 방향을 설정합니다.
     */
    void Editor::FEditorViewportClient::SetViewType(Editor::EEditorViewportType Type) {
        if (Type < Editor::EEditorViewportType::Perspective || Type > Editor::EEditorViewportType::Right || mViewType == Type || mGizmo.IsDragging()) {
            return;
        }

        mFocusActive = false;

        if (mViewType == Editor::EEditorViewportType::Perspective) {
            mPerspectiveLocation = mCameraLocation;
            mPerspectiveRotation = mCameraRotation;
            mPerspectivePivot = mOrbitPivot;
        }

        mViewType = Type;

        if (Type == Editor::EEditorViewportType::Perspective) {
            mCameraLocation = mPerspectiveLocation;
            mCameraRotation = mPerspectiveRotation;
            mOrbitPivot = mPerspectivePivot;

            return;
        }

        switch (Type) {
        case Editor::EEditorViewportType::Top:
            mCameraRotation = FRotator{-90_deg, 0_deg, 0_deg};

            break;
        case Editor::EEditorViewportType::Bottom:
            mCameraRotation = FRotator{90_deg, 0_deg, 0_deg};

            break;
        case Editor::EEditorViewportType::Front:
            mCameraRotation = FRotator{0_deg, 180_deg, 0_deg};

            break;
        case Editor::EEditorViewportType::Back:
            mCameraRotation = {};

            break;
        case Editor::EEditorViewportType::Left:
            mCameraRotation = FRotator{0_deg, -90_deg, 0_deg};

            break;
        case Editor::EEditorViewportType::Right:
            mCameraRotation = FRotator{0_deg, 90_deg, 0_deg};

            break;
        default:
            break;
        }

        mCameraLocation = mOrbitPivot - mCameraRotation.Vector() * (5000_m).GetBase();
    }

    /**
     * @brief 원근 또는 정사영 뷰 방향을 반환합니다.
     */
    Editor::EEditorViewportType Editor::FEditorViewportClient::GetViewType() const {
        return mViewType;
    }

    /**
     * @brief 현재 뷰 방향의 표시 이름을 반환합니다.
     */
    const char* Editor::FEditorViewportClient::GetViewTypeName() const {
        switch (mViewType) {
        case Editor::EEditorViewportType::Top:
            return "Top";
        case Editor::EEditorViewportType::Bottom:
            return "Bottom";
        case Editor::EEditorViewportType::Front:
            return "Front";
        case Editor::EEditorViewportType::Back:
            return "Back";
        case Editor::EEditorViewportType::Left:
            return "Left";
        case Editor::EEditorViewportType::Right:
            return "Right";
        default:
            return GetPreviewCamera() != nullptr ? "Camera Cut" : "Perspective";
        }
    }

    /**
     * @brief 활성 뷰포트의 기본 배치 위치를 반환합니다.
     */
    FVector Editor::FEditorViewportClient::GetPlacementLocation() const {
        if (const Engine::UCameraComponent* Camera{GetPreviewCamera()}) {
            const FTransform& Transform{Camera->GetTransform()};

            return Transform.mLocation + Transform.mRotation.ForwardVector() * (6_m).GetBase();
        }

        return mViewType == Editor::EEditorViewportType::Perspective ? mCameraLocation + mCameraRotation.Vector() * (6_m).GetBase() : mOrbitPivot;
    }

    /**
     * @brief 에디터 카메라의 월드 위치를 반환합니다.
     */
    const FVector& Editor::FEditorViewportClient::GetCameraLocation() const {
        return mCameraLocation;
    }

    /**
     * @brief 편집 카메라의 현재 오일러 회전을 반환합니다.
     */
    const FRotator& Editor::FEditorViewportClient::GetCameraRotation() const {
        return mCameraRotation;
    }

    /**
     * @brief 정사영 전환 중에도 보관한 원근 카메라를 반환합니다.
     */
    Engine::FEditorCameraState Editor::FEditorViewportClient::GetPerspectiveCamera() const {
        Engine::FEditorCameraState State{};
        State.Set(mViewType == Editor::EEditorViewportType::Perspective ? mCameraLocation : mPerspectiveLocation, mViewType == Editor::EEditorViewportType::Perspective ? mCameraRotation : mPerspectiveRotation, mFieldOfView, mNearClip, mFarClip);

        return State;
    }

    /**
     * @brief 원근 카메라를 복원합니다.
     */
    bool Editor::FEditorViewportClient::SetPerspectiveCamera(const Engine::FEditorCameraState& State) {
        if (!State.IsValid()) {
            return false;
        }

        mFocusActive = false;
        mPerspectiveLocation = State.GetLocation();
        mPerspectiveRotation = State.GetRotation();
        mPerspectivePivot = mPerspectiveLocation + mPerspectiveRotation.Vector() * (10_m).GetBase();
        mFieldOfView = State.GetFieldOfView();
        mNearClip = State.GetNearClip();
        mFarClip = State.GetFarClip();

        if (mViewType == Editor::EEditorViewportType::Perspective) {
            mCameraLocation = mPerspectiveLocation;
            mCameraRotation = mPerspectiveRotation;
            mOrbitPivot = mPerspectivePivot;
        }

        return true;
    }

    /**
     * @brief 뷰포트 변환 편집 기즈모를 반환합니다.
     */
    Editor::FTransformGizmo& Editor::FEditorViewportClient::GetGizmo() {
        return mGizmo;
    }

    /**
     * @brief 이 뷰포트의 기즈모 스냅과 크기 설정을 읽기 전용으로 조회합니다.
     */
    const Editor::FGizmoSettings* Editor::FEditorViewportClient::GetGizmoSettings() const {
        return mGizmo.GetSettings();
    }

    /**
     * @brief 기즈모 도구와 설정을 변경할 수 있는지 검사합니다.
     */
    bool Editor::FEditorViewportClient::CanEditGizmoSettings() const {
        return !mGizmo.IsDragging() && mContext->CanEditWorld();
    }

    /**
     * @brief 편집 가능 여부와 값의 범위를 검증하여 기즈모 수치 설정을 변경합니다.
     */
    bool Editor::FEditorViewportClient::SetGizmoSetting(Editor::EGizmoSetting Setting, float Value) {
        if (Setting == Editor::EGizmoSetting::TranslationStep) {
            return SetTranslationSnapStep(Core::FLength::FromBase(Value));
        }

        if (Setting == Editor::EGizmoSetting::RotationStep) {
            return SetRotationSnapStep(Value * 1_deg);
        }

        if (!CanEditGizmoSettings() || !std::isfinite(Value)) {
            return false;
        }

        Editor::FGizmoSettings Settings{*mGizmo.GetSettings()};

        switch (Setting) {
        case Editor::EGizmoSetting::ScaleStep:
            if (Value <= 0.0f) {
                return false;
            }

            Settings.mScaleStep = Value;

            break;
        case Editor::EGizmoSetting::Size:
            Settings.mSize = std::clamp(Value, 0.5f, 2.0f);

            break;
        default:
            return false;
        }

        mGizmo.SetSettings(Settings);

        return true;
    }

    float Editor::FEditorViewportClient::GetGizmoSettingValue(Editor::EGizmoSetting Setting) const {
        const Editor::FGizmoSettings& Settings{*mGizmo.GetSettings()};

        switch (Setting) {
        case Editor::EGizmoSetting::TranslationStep:
            return Settings.mTranslationStep.GetBase();
        case Editor::EGizmoSetting::RotationStep:
            return Core::Math::ToDegrees(Settings.mRotationStep);
        case Editor::EGizmoSetting::ScaleStep:
            return Settings.mScaleStep;
        case Editor::EGizmoSetting::Size:
            return Settings.mSize;
        default:
            return 0.0f;
        }
    }

    bool Editor::FEditorViewportClient::SetTranslationSnapStep(Core::FLength Step) {
        if (!CanEditGizmoSettings() || !Step.IsFinite() || Step <= 0_m) {
            return false;
        }

        Editor::FGizmoSettings Settings{*mGizmo.GetSettings()};
        Settings.mTranslationStep = Step;
        mGizmo.SetSettings(Settings);

        return true;
    }

    bool Editor::FEditorViewportClient::SetRotationSnapStep(Core::FAngle Step) {
        if (!CanEditGizmoSettings() || !Step.IsFinite() || Step <= 0_deg) {
            return false;
        }

        Editor::FGizmoSettings Settings{*mGizmo.GetSettings()};
        Settings.mRotationStep = Step;
        mGizmo.SetSettings(Settings);

        return true;
    }

    /**
     * @brief 실행 검사를 통과한 스냅 전환 명령을 기즈모 설정에 반영합니다.
     */
    bool Editor::FEditorViewportClient::ToggleGizmoSetting(Editor::EViewportCommand Command) {
        Editor::FGizmoSettings Settings{*mGizmo.GetSettings()};

        switch (Command) {
        case Editor::EViewportCommand::ToggleTranslationSnap:
            Settings.mTranslationSnap = !Settings.mTranslationSnap;

            break;
        case Editor::EViewportCommand::ToggleRotationSnap:
            Settings.mRotationSnap = !Settings.mRotationSnap;

            break;
        case Editor::EViewportCommand::ToggleScaleSnap:
            Settings.mScaleSnap = !Settings.mScaleSnap;

            break;
        case Editor::EViewportCommand::ToggleTransformSnap:
            return mGizmoMode == Editor::EGizmoMode::Translate ? ToggleGizmoSetting(Editor::EViewportCommand::ToggleTranslationSnap) : mGizmoMode == Editor::EGizmoMode::Rotate ? ToggleGizmoSetting(Editor::EViewportCommand::ToggleRotationSnap) : mGizmoMode == Editor::EGizmoMode::Scale && ToggleGizmoSetting(Editor::EViewportCommand::ToggleScaleSnap);
        default:
            return false;
        }

        mGizmo.SetSettings(Settings);

        return true;
    }

    /**
     * @brief 변환 모드를 저장하고 스케일을 선택하면 로컬 좌표계를 적용합니다.
     */
    void Editor::FEditorViewportClient::SetGizmoMode(Editor::EGizmoMode Mode) {
        mGizmoMode = Mode;

        if (mGizmoMode == Editor::EGizmoMode::Scale) {
            mGizmoSpace = Editor::EGizmoSpace::Local;
        }
    }

    /**
     * @brief 클라이언트가 관리하는 변환 기즈모 모드를 반환합니다.
     */
    Editor::EGizmoMode Editor::FEditorViewportClient::GetGizmoMode() const {
        return mGizmoMode;
    }

    /**
     * @brief 명령 실행 경로에서 검증한 기즈모 좌표계를 저장합니다.
     */
    void Editor::FEditorViewportClient::SetGizmoSpace(Editor::EGizmoSpace Space) {
        mGizmoSpace = Space;
    }

    /**
     * @brief 클라이언트가 관리하는 기즈모 축의 좌표 공간을 반환합니다.
     */
    Editor::EGizmoSpace Editor::FEditorViewportClient::GetGizmoSpace() const {
        return mGizmoSpace;
    }

    /**
     * @brief 기즈모 메시를 우선 검사하고 공통 CPU 광선 질의로 메시 컴포넌트의 핸들을 구합니다.
     */
    Engine::FHitProxy Editor::FEditorViewportClient::GetHitProxy(const Engine::FSceneViewport* Viewport, const FVector2& Position) const {
        Engine::UWorld* World{mContext->GetWorld()};

        if (Viewport == nullptr || World == nullptr || mViewMode == Engine::EViewMode::PhysicsBodies) {
            return {};
        }

        const Engine::FSceneView View{CalcSceneView(Viewport->GetSize())};
        const Engine::FHitProxy GizmoHit{mGizmo.HitTest(Position, View)};

        if (GizmoHit.mType != Engine::EHitProxyType::None) {
            return GizmoHit;
        }

        FRay Ray{};

        if (!View.Deproject(Position, Ray)) {
            return {};
        }

        Engine::FHitProxy Result{};
        float Closest{std::numeric_limits<float>::max()};

        if (World->GetPersistentLevel() == nullptr || mContext->GetRenderer() == nullptr) {
            return Result;
        }

        const FVector Right{View.mView(0, 0), View.mView(1, 0), View.mView(2, 0)};
        const FVector Up{View.mView(0, 1), View.mView(1, 1), View.mView(2, 1)};

        for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
            if (!Actor->IsVisible()) {
                continue;
            }

            Engine::FPrimitiveSceneProxy Icon{};

            if (TryCreateActorIcon(*Actor, Icon) && Engine::RaycastMesh(&Engine::GetPrimitiveMesh(Engine::EPrimitiveType::Plane), Engine::MakeSpriteWorld(Icon.mWorld, *Icon.mSprite, View), Ray, Closest)) {
                Result = {Engine::EHitProxyType::Object, CoreUObject::MakeObjectRef(Actor)};
            }

            for (const auto& Component : Actor->GetComponents()) {
                if (auto* Light{CoreUObject::Cast<Engine::ULightComponentBase>(Component.get())}; Light != nullptr && mShowFlags.mLightIcons && !Actor->IsPendingDestroy()) {
                    const Engine::FPrimitiveSceneProxy Proxy{Light->CreateLightProxy().CreateIconProxy()};

                    if (Engine::RaycastMesh(&Engine::GetPrimitiveMesh(Engine::EPrimitiveType::Plane), Engine::MakeSpriteWorld(Proxy.mWorld, *Proxy.mSprite, View), Ray, Closest)) {
                        Result = {Engine::EHitProxyType::Object, CoreUObject::MakeObjectRef(Light)};
                    }
                }

                if (auto* Fog{CoreUObject::Cast<Engine::UHeightFogComponent>(Component.get())}; Fog != nullptr && mShowFlags.mFogIcons && !Actor->IsPendingDestroy()) {
                    const Engine::FPrimitiveSceneProxy Proxy{Fog->CreateFogProxy().CreateIconProxy()};

                    if (Engine::RaycastMesh(&Engine::GetPrimitiveMesh(Engine::EPrimitiveType::Plane), Engine::MakeSpriteWorld(Proxy.mWorld, *Proxy.mSprite, View), Ray, Closest)) {
                        Result = {Engine::EHitProxyType::Object, CoreUObject::MakeObjectRef(Fog)};
                    }
                }

                auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(Component.get())};
                auto* Sprite{CoreUObject::Cast<Engine::UBillboardComponent>(Component.get())};
                auto* Text{CoreUObject::Cast<Engine::UBillboardTextComponent>(Component.get())};

                if (!mShowFlags.mPrimitives || Actor->IsPendingDestroy() || Primitive == nullptr || !Primitive->ShouldCreateRenderState()) {
                    continue;
                }

                if (Sprite != nullptr) {
                    const Engine::FPrimitiveSceneProxy Proxy{Sprite->CreateSceneProxy()};

                    if (mShowFlags.mSprites && Engine::RaycastMesh(&Engine::GetPrimitiveMesh(Engine::EPrimitiveType::Plane), Engine::MakeSpriteWorld(Proxy.mWorld, *Proxy.mSprite, View), Ray, Closest)) {
                        Result = {Engine::EHitProxyType::Object, CoreUObject::MakeObjectRef(Sprite)};
                    }

                    continue;
                }

                if (Text == nullptr) {
                    if (Engine::RaycastMesh(&Primitive->GetMeshData(), Primitive->GetWorldMatrix(), Ray, Closest)) {
                        Result = {Engine::EHitProxyType::Object, CoreUObject::MakeObjectRef(Primitive)};
                    }

                    continue;
                }

                FVector2 Minimum{};
                FVector2 Maximum{};

                if (!mShowFlags.mText || !mContext->GetRenderer()->GetTextBounds(Text->GetText(), Minimum, Maximum)) {
                    continue;
                }

                const FTransform& Transform{Text->GetTransform()};
                const FVector4 Clip{FVector4{Transform.mLocation, 1.0f} * View.mViewProjection};
                FVector Intersection{};

                if (Clip.w <= 0.0f || Clip.z < 0.0f || Clip.z > Clip.w || !Core::Math::IntersectPlane(Ray, Transform.mLocation, View.mForward, Intersection)) {
                    continue;
                }

                const float Distance{(Intersection - Ray.mOrigin).Dot(Ray.mDirection)};
                const FVector Offset{Intersection - Transform.mLocation};
                const float Horizontal{Offset.Dot(Right) / (Text->GetTextSize() * std::abs(Transform.mScale.y))};
                const float Vertical{Offset.Dot(Up) / (Text->GetTextSize() * std::abs(Transform.mScale.z))};

                if (Distance < Closest && Horizontal >= Minimum.x && Horizontal <= Maximum.x && Vertical >= Minimum.y && Vertical <= Maximum.y) {
                    Closest = Distance;
                    Result = {Engine::EHitProxyType::Object, CoreUObject::MakeObjectRef(Text)};
                }
            }
        }

        return Result;
    }

    /**
     * @brief 텍스트·Billboard의 뷰 정렬을 반영한 월드 AABB를 계산합니다.
     */
    FBox Editor::FEditorViewportClient::CalcComponentBounds(const Engine::UPrimitiveComponent* Component, const Engine::FSceneView& View) const {
        if (auto* Sprite{CoreUObject::Cast<Engine::UBillboardComponent>(Component)}) {
            const Engine::FPrimitiveSceneProxy Proxy{Sprite->CreateSceneProxy()};

            return FBox{{-(0.5_m).GetBase(), -(0.5_m).GetBase(), -(0.001_m).GetBase()}, {(0.5_m).GetBase(), (0.5_m).GetBase(), (0.001_m).GetBase()}}.Transformed(Engine::MakeSpriteWorld(Proxy.mWorld, *Proxy.mSprite, View));
        }

        auto* Text{CoreUObject::Cast<Engine::UBillboardTextComponent>(Component)};
        FVector2 Minimum{};
        FVector2 Maximum{};

        if (Text == nullptr || mContext->GetRenderer() == nullptr || !mContext->GetRenderer()->GetTextBounds(Text->GetText(), Minimum, Maximum)) {
            return Component->GetWorldBounds();
        }

        const FTransform& Transform{Text->GetTransform()};
        const FVector Right{View.mView(0, 0), View.mView(1, 0), View.mView(2, 0)};
        const FVector Up{View.mView(0, 1), View.mView(1, 1), View.mView(2, 1)};
        const float WidthScale{Text->GetTextSize() * std::abs(Transform.mScale.y)};
        const float HeightScale{Text->GetTextSize() * std::abs(Transform.mScale.z)};
        const FVector Center{Transform.mLocation + Right * ((Minimum.x + Maximum.x) * 0.5f * WidthScale) + Up * ((Minimum.y + Maximum.y) * 0.5f * HeightScale)};
        const float HalfWidth{(Maximum.x - Minimum.x) * 0.5f * WidthScale};
        const float HalfHeight{(Maximum.y - Minimum.y) * 0.5f * HeightScale};
        const FVector Extent{std::abs(Right.x) * HalfWidth + std::abs(Up.x) * HalfHeight, std::abs(Right.y) * HalfWidth + std::abs(Up.y) * HalfHeight, std::abs(Right.z) * HalfWidth + std::abs(Up.z) * HalfHeight};

        return {Center - Extent, Center + Extent};
    }

    /**
     * @brief 드롭한 메시 에셋을 뷰포트 위치에 배치합니다.
     */
    bool Editor::FEditorViewportClient::DropMesh(const std::filesystem::path& Path, const FVector2& Position, const FVector2& Size) {
        Engine::UWorld* World{mContext->GetWorld()};
        Engine::FProject* Project{mContext->GetProject()};

        if (!mContext->CanEditWorld() || mGizmo.IsDragging() || World == nullptr || Project == nullptr) {
            return false;
        }

        const Engine::FSceneView View{CalcSceneView(Size)};
        FRay Ray{};

        if (!View.Deproject(Position, Ray)) {
            return false;
        }

        Engine::FWorldHit Hit{};
        FVector Location{mViewType == Editor::EEditorViewportType::Perspective ? Ray.mOrigin + Ray.mDirection * (6_m).GetBase() : mOrbitPivot};
        const bool OnSurface{World->Raycast(Ray, Hit)};
        const bool OnGround{!OnSurface && Core::Math::IntersectPlane(Ray, mViewType == Editor::EEditorViewportType::Perspective ? FVector{} : mOrbitPivot, mViewType == Editor::EEditorViewportType::Perspective ? FVector{0.0f, 0.0f, 1.0f} : View.mForward, Location)};

        if (OnSurface) {
            Location = Hit.mPosition;
        }

        Core::FString Error{};
        Asset::FAssetManager* Assets{Project->GetAssets()};
        Asset::EAssetType Type{};

        if (!Asset::FAssetFile::GetType(Assets->ResolvePath(Path, Error), Type, Error)) {
            mContext->AddLog(Error);

            return false;
        }

        // 같은 드롭 동작에서 에셋 종류를 판별하고 바인드 경계를 기준으로 바닥에 놓습니다.
        Asset::USkeletalMesh* Skeletal{Type == Asset::EAssetType::SkeletalMesh ? Assets->LoadSkeletalMesh(Path, Error) : nullptr};
        const Asset::UStaticMesh* Mesh{Skeletal != nullptr ? Skeletal->GetGeometry() : Type == Asset::EAssetType::Mesh ? Assets->LoadMesh(Path, Error) : nullptr};

        if (Mesh == nullptr) {
            mContext->AddLog(Error);

            return false;
        }

        if (OnSurface || (OnGround && mViewType == Editor::EEditorViewportType::Perspective)) {
            Location.z -= Mesh->GetBounds().mMinimum.z;
        }

        const FTransform Placement{Location, {}, {1.0f, 1.0f, 1.0f}};

        return Skeletal != nullptr ? mContext->SpawnSkeletalMesh(Path, Placement) != nullptr : mContext->SpawnMesh(Path, Placement) != nullptr;
    }

    /**
     * @brief 장면의 Lit 또는 Wireframe 표시 모드를 반환합니다.
     */
    Engine::EViewMode Editor::FEditorViewportClient::GetViewMode() const {
        return mViewMode;
    }

    /**
     * @brief 장면의 Lit 또는 Wireframe 표시 모드를 설정합니다.
     */
    void Editor::FEditorViewportClient::SetViewMode(Engine::EViewMode Mode) {
        if (!SupportsViewMode(Mode)) {
            mViewMode = mColorViewMode;

            return;
        }

        if (Mode == Engine::EViewMode::PhysicsBodies) {
            mGizmo.Cancel();
            mViewMode = Mode;
        } else if (Engine::IsBufferVisualization(Mode)) {
            mViewMode = Mode;
        } else if (Mode == Engine::EViewMode::Lit_Phong || Mode == Engine::EViewMode::Lit_Gouraud || Mode == Engine::EViewMode::Lit_Lambert || Mode == Engine::EViewMode::Unlit || Mode == Engine::EViewMode::Wireframe) {
            mViewMode = Mode;
            mColorViewMode = Mode;
        }
    }

    bool Editor::FEditorViewportClient::SupportsViewMode(Engine::EViewMode Mode) const {
        const RenderCore::IRendererModule* Renderer{mContext != nullptr ? mContext->GetRenderer() : nullptr};

        return Renderer == nullptr || !Renderer->UsesForwardShading() || !Engine::RequiresDeferredShading(Mode);
    }

    /**
     * @brief 버퍼 시각화에서 Color로 돌아갈 때 사용할 마지막 컬러 표시 모드를 반환합니다.
     */
    Engine::EViewMode Editor::FEditorViewportClient::GetColorViewMode() const {
        return mColorViewMode;
    }

    /**
     * @brief 뷰포트별 장면·보조 표시 필터를 반환합니다.
     */
    const Engine::FEngineShowFlags& Editor::FEditorViewportClient::GetShowFlags() const {
        return mShowFlags;
    }

    /**
     * @brief 진행 중인 기즈모 조작을 취소하고 이 뷰포트의 표시 필터를 교체합니다.
     */
    void Editor::FEditorViewportClient::SetShowFlags(const Engine::FEngineShowFlags& Flags) {
        mGizmo.Cancel();
        mShowFlags = Flags;
    }

    /**
     * @brief 축과 AABB 선의 픽셀 두께를 반환합니다.
     */
    float Editor::FEditorViewportClient::GetLineThickness() const {
        return mLineThickness;
    }

    /**
     * @brief 축과 AABB 선의 픽셀 두께를 설정합니다.
     */
    bool Editor::FEditorViewportClient::SetLineThickness(float Thickness) {
        if (!std::isfinite(Thickness) || Thickness < 0.5f || Thickness > 10.0f) {
            return false;
        }

        mLineThickness = Thickness;

        return true;
    }

    /**
     * @brief 그리드 선 사이의 월드 거리를 반환합니다.
     */
    float Editor::FEditorViewportClient::GetGridSpacing() const {
        return mGridSpacing;
    }

    /**
     * @brief 그리드 선 사이의 월드 거리를 설정합니다.
     */
    bool Editor::FEditorViewportClient::SetGridSpacing(float Spacing) {
        if (!std::isfinite(Spacing) || Spacing < (0.01_m).GetBase() || Spacing > (1000_m).GetBase()) {
            return false;
        }

        mGridSpacing = Spacing;

        return true;
    }

    /**
     * @brief 마우스 이동 1픽셀당 카메라 회전 각도를 반환합니다.
     */
    float Editor::FEditorViewportClient::GetCameraSensitivity() const {
        return mCameraSensitivity;
    }

    /**
     * @brief 마우스 이동 1픽셀당 카메라 회전 각도를 설정합니다.
     */
    bool Editor::FEditorViewportClient::SetCameraSensitivity(float Sensitivity) {
        if (!std::isfinite(Sensitivity) || Sensitivity < Core::Math::ToDegrees(0.01_deg) || Sensitivity > Core::Math::ToDegrees(2_deg)) {
            return false;
        }

        mCameraSensitivity = Sensitivity;

        return true;
    }

    /**
     * @brief 자유 시점 카메라의 기본 이동 속도를 반환합니다.
     */
    Core::FSpeed Editor::FEditorViewportClient::GetCameraSpeed() const {
        return mCameraSpeed;
    }

    /**
     * @brief 자유 시점 카메라의 기본 이동 속도를 설정합니다.
     */
    bool Editor::FEditorViewportClient::SetCameraSpeed(Core::FSpeed Speed) {
        if (!Speed.IsFinite() || Speed < 0.1_mps || Speed > 128_mps) {
            return false;
        }

        mCameraSpeed = Speed;

        return true;
    }

    /**
     * @brief 정사영 뷰의 가로 가시 범위를 반환합니다.
     */
    float Editor::FEditorViewportClient::GetOrthographicWidth() const {
        return mOrthographicWidth;
    }

    /**
     * @brief 0.01~100000 범위의 유한한 정사영 가로 폭을 적용합니다.
     */
    bool Editor::FEditorViewportClient::SetOrthographicWidth(float Width) {
        if (!std::isfinite(Width) || Width < (0.01_m).GetBase() || Width > (100000_m).GetBase()) {
            return false;
        }

        mFocusActive = false;
        mOrthographicWidth = Width;

        return true;
    }

    /**
     * @brief 드롭한 액터 프리셋을 광선 적중점이나 배치 평면에 생성합니다.
     */
    bool Editor::FEditorViewportClient::DropActor(const Core::FString& Preset, const FVector2& Position, const FVector2& Size) {
        Engine::UWorld* World{mContext->GetWorld()};

        if (!mContext->CanEditWorld() || mGizmo.IsDragging() || World == nullptr) {
            return false;
        }

        const Engine::FSceneView View{CalcSceneView(Size)};
        FRay Ray{};

        if (!View.Deproject(Position, Ray)) {
            return false;
        }

        FVector Location{mViewType == Editor::EEditorViewportType::Perspective ? Ray.mOrigin + Ray.mDirection * (6_m).GetBase() : mOrbitPivot};
        Engine::FWorldHit Hit{};

        if (World->Raycast(Ray, Hit)) {
            Location = Hit.mPosition;
        } else {
            Core::Math::IntersectPlane(Ray, mViewType == Editor::EEditorViewportType::Perspective ? FVector{} : mOrbitPivot, mViewType == Editor::EEditorViewportType::Perspective ? FVector{0.0f, 0.0f, 1.0f} : View.mForward, Location);
        }

        if (Preset == "ADecalActor") {
            return mContext->SpawnActorPreset(Preset, FTransform{Location, Ray.mDirection.Rotation().Quaternion(), {1.0f, 1.0f, 1.0f}}) != nullptr;
        }

        const bool Light{Preset.find("Light") != Core::FString::npos};
        const FRotator Rotation{Preset.find("Directional") != Core::FString::npos || Preset.find("Spot") != Core::FString::npos ? FRotator{-45_deg, 0_deg, 0_deg} : FRotator{}};
        const FVector Offset{0.0f, 0.0f, Light ? (1_m).GetBase() : Preset == "Cube" || Preset == "Sphere" || Preset == "AStaticMeshActor" ? (0.5_m).GetBase() : 0.0f};

        return mContext->SpawnActorPreset(Preset, FTransform{Location + Offset, Rotation.Quaternion(), {1.0f, 1.0f, 1.0f}}) != nullptr;
    }

}
