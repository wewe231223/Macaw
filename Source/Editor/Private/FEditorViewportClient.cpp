/**
 * @file FEditorViewportClient.cpp
 * @brief 에디터 카메라, 선택, 공통 명령과 기즈모 입력을 처리합니다.
 * @ingroup Editor
 */

#include "Editor/FEditorViewportClient.h"
#include "Engine/Component/UHeightFogComponent.h"
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
#include "Engine/ULevel.h"
#include "Engine/AActor.h"
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
#include "Engine/FViewportInput.h"
#include "Engine/FSceneViewport.h"
#include "Engine/FWorldHit.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Editor/FEditorContext.h"
#include "Editor/FViewportCommandInfo.h"
#include "Engine/FProject.h"
#include "Asset/UStaticMesh.h"
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
		  mGizmo{Context, this} {
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Editor::FEditorViewportClient::~FEditorViewportClient() = default;

    void Editor::FEditorViewportClient::SetEditorContext(Editor::FEditorContext* Context) {
        if (Context == nullptr || Context == mContext) {
            return;
        }

        mGizmo.SetEditorContext(Context);
        mContext = Context;
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
        if (Viewport == nullptr) {
            return;
        }

        const Core::FVector2 Size{Viewport->GetSize()};

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

        if (NavigateCamera(Input, Size, DeltaTime)) {
            return;
        }

        if (Input.mDelete && Input.mFocused) {
            mContext->DestroySelectedActor();
            return;
        }

        if (Input.mLeftPressed && Input.mHovered && !Input.mAlt && mContext->GetWorld() != nullptr) {
            if (Hit.mType == Engine::EHitProxyType::Object) {
                CoreUObject::UObject* Object{mContext->ResolveObject(Hit.mObject)};
                const auto* Component{CoreUObject::Cast<Engine::UActorComponent>(Object)};
                Engine::AActor* Actor{Component != nullptr ? Component->GetOwner() : CoreUObject::Cast<Engine::AActor>(Object)};

                if (Actor != nullptr) {
                    if (Input.mControl) {
                        mContext->ToggleActorSelection(Actor->GetHandle());
                    } else {
                        mContext->SelectActor(Actor->GetHandle());
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
    bool Editor::FEditorViewportClient::NavigateCamera(const Engine::FViewportInput& Input, const Core::FVector2& Size, float DeltaTime) {
        if (!Input.mHovered && !Input.mFocused) {
            return false;
        }

        if (Input.mFocusSelection && Input.mFocused && !Input.mRightDown && !Input.mAlt) {
            FocusSelection(Size);
        }

        const Core::FQuat Rotation{mCameraRotation.Quaternion()};

        if (mViewType != Editor::EEditorViewportType::Perspective) {
            if (Input.mWheel != 0.0f && Input.mHovered) {
                const float PreviousWidth{mOrthographicWidth};

                mOrthographicWidth = std::clamp(mOrthographicWidth * std::pow(0.85f, Input.mWheel), 0.1f, 10000.0f);

                const float PixelScale{(PreviousWidth - mOrthographicWidth) / std::max(Size.x, 1.0f)};
                const Core::FVector Offset{Rotation.RightVector() * ((Input.mMouse.x - Size.x * 0.5f) * PixelScale) + Rotation.UpVector() * ((Size.y * 0.5f - Input.mMouse.y) * PixelScale)};
                mCameraLocation += Offset;
                mOrbitPivot += Offset;
            }

            if (Input.mRightDown || Input.mMiddleDown) {
                const float PixelScale{mOrthographicWidth / std::max(Size.x, 1.0f)};
                const Core::FVector Offset{(Rotation.RightVector() * -Input.mDelta.x + Rotation.UpVector() * Input.mDelta.y) * PixelScale};
                mCameraLocation += Offset;
                mOrbitPivot += Offset;

                return true;
            }

            return Input.mAlt || (Input.mWheel != 0.0f && Input.mHovered);
        }

        if (Input.mWheel != 0.0f && Input.mHovered) {
            if (Input.mRightDown && !Input.mAlt) {
                SetCameraSpeed(std::clamp(mCameraSpeed * std::pow(1.25f, Input.mWheel), 0.1f, 128.0f));
            } else {
                mCameraLocation += Rotation.ForwardVector() * Input.mWheel * mCameraSpeed * 0.5f;
            }
        }

        if (Input.mMiddleDown || (Input.mLeftDown && Input.mRightDown && !Input.mAlt)) {
            const float Distance{std::max(mCameraLocation.Distance(mOrbitPivot), 1.0f)};
            const float PixelScale{2.0f * Distance * Core::Math::Tan(30_deg) / std::max(Size.y, 1.0f)};
            const Core::FVector Pan{Rotation * Core::FVector{0.0f, -Input.mDelta.x, Input.mDelta.y} * PixelScale};
            mCameraLocation += Pan;
            mOrbitPivot += Pan;

            return true;
        }

        if (Input.mAlt && (Input.mLeftDown || Input.mRightDown)) {
            float Distance{std::max(mCameraLocation.Distance(mOrbitPivot), 0.1f)};
            Core::FVector Forward{mOrbitPivot - mCameraLocation};

            if (Forward.Normalize()) {
                const Core::FRotator Facing{Forward.Rotation()};
                mCameraRotation.yaw = Facing.yaw;
                mCameraRotation.pitch = std::clamp(Facing.pitch, -89.0f, 89.0f);
            }

            if (Input.mLeftDown) {
                mCameraRotation.yaw += Input.mDelta.x * mCameraSensitivity;
                mCameraRotation.pitch = std::clamp(mCameraRotation.pitch - Input.mDelta.y * mCameraSensitivity, -89.0f, 89.0f);
            } else {
                Distance = std::clamp(Distance * std::exp((Input.mDelta.y - Input.mDelta.x) * 0.01f), 0.1f, 100000.0f);
            }

            mCameraLocation = mOrbitPivot - mCameraRotation.Vector() * Distance;

            return true;
        }

        if (Input.mRightDown) {
            const float OrbitDistance{std::max(mCameraLocation.Distance(mOrbitPivot), 0.1f)};

            mCameraRotation.yaw += Input.mDelta.x * mCameraSensitivity;
            mCameraRotation.pitch = std::clamp(mCameraRotation.pitch - Input.mDelta.y * mCameraSensitivity, -89.0f, 89.0f);

            const Core::FQuat FlightRotation{mCameraRotation.Quaternion()};
            const Core::FVector LocalMovement{static_cast<float>(Input.mForward) - static_cast<float>(Input.mBackward), static_cast<float>(Input.mRight) - static_cast<float>(Input.mLeft), 0.0f};
            Core::FVector Movement{FlightRotation * LocalMovement};
            Movement.z += static_cast<float>(Input.mUp) - static_cast<float>(Input.mDown);

            mCameraLocation += Movement.Normalized() * std::max(DeltaTime, 0.0f) * mCameraSpeed * (Input.mFast ? 3.0f : 1.0f);
            mOrbitPivot = mCameraLocation + FlightRotation.ForwardVector() * OrbitDistance;

            return true;
        }

        return Input.mAlt || Input.mMiddleDown || (Input.mWheel != 0.0f && Input.mHovered);
    }

    /**
     * @brief 선택한 액터가 화면에 들어오도록 카메라 위치와 중심을 조정합니다.
     */
    void Editor::FEditorViewportClient::FocusSelection(const Core::FVector2& Size) {
        const Engine::AActor* Actor{mContext->GetSelectedActor()};
        const auto* Component{Actor != nullptr ? CoreUObject::Cast<Engine::UPrimitiveComponent>(Actor->GetRootComponent()) : nullptr};

        const auto* Light{Actor != nullptr ? CoreUObject::Cast<Engine::ULightComponentBase>(Actor->GetRootComponent()) : nullptr};

        const auto* Fog{Actor != nullptr ? CoreUObject::Cast<Engine::UHeightFogComponent>(Actor->GetRootComponent()) : nullptr};

        if (Component == nullptr && Light == nullptr && Fog == nullptr) {
            return;
        }

        const Core::FVector Location{Light != nullptr ? Light->GetTransform().mLocation : Fog != nullptr ? Fog->GetTransform().mLocation : Core::FVector{}};
        const Core::FBox Bounds{Component != nullptr ? CalcComponentBounds(Component, CalcSceneView(Size)) : Core::FBox{Location - Core::FVector{0.5f, 0.5f, 0.5f}, Location + Core::FVector{0.5f, 0.5f, 0.5f}}};
        mOrbitPivot = Bounds.mMinimum.Lerp(Bounds.mMaximum, 0.5f);

        const float Radius{Bounds.mMinimum.Distance(Bounds.mMaximum) * 0.5f};

        if (mViewType != Editor::EEditorViewportType::Perspective) {
            const float Aspect{std::max(Size.x, 1.0f) / std::max(Size.y, 1.0f)};
            mOrthographicWidth = std::clamp(Radius * 2.4f * std::max(Aspect, 1.0f), 0.1f, 10000.0f);
            mCameraLocation = mOrbitPivot - mCameraRotation.Vector() * 5000.0f;
            return;
        }

        const Core::FAngle HalfFieldOfView{Core::Math::Atan2(Core::Math::Tan(mFieldOfView * 0.5_deg) * std::min(std::max(Size.x, 1.0f) / std::max(Size.y, 1.0f), 1.0f), 1.0f)};
        const float Distance{std::max(0.5f, Radius / Core::Math::Sin(HalfFieldOfView) * 1.2f)};

        mCameraLocation = mOrbitPivot - mCameraRotation.Vector() * Distance;
    }

    /**
     * @brief 월드 원점에 고정한 그리드와 에디터 보조 요소를 포함한 뷰포트를 그립니다.
     */
    void Editor::FEditorViewportClient::Draw(Engine::FSceneViewport* Viewport) {
        Engine::UWorld* World{mContext->GetWorld()};

        if (World == nullptr || World->GetScene() == nullptr || mContext->GetRenderer() == nullptr) {
            return;
        }

        const Engine::FSceneView View{CalcSceneView(Viewport->GetSize())};

        mDrawInterface.Clear();

        if (mShowFlags.mGrid) {
            const float Spacing{mGridSpacing};
            const float Extent{50.0f * Spacing};

            for (Core::Int32 Index{-50}; Index <= 50; ++Index) {
                const float Offset{Index * Spacing};
                const Core::FVector Color{Index % 10 == 0 ? Core::FVector{0.22f, 0.25f, 0.30f} : Core::FVector{0.13f, 0.15f, 0.19f}};
                mDrawInterface.DrawLine({Offset, -Extent, 0.0f}, {Offset, Extent, 0.0f}, Color, true);
                mDrawInterface.DrawLine({-Extent, Offset, 0.0f}, {Extent, Offset, 0.0f}, Color, true);
            }
        }

        if (mShowFlags.mAxes) {
            mDrawInterface.DrawLine({}, {2.0f, 0.0f, 0.0f}, {1.0f, 0.25f, 0.2f}, true, mLineThickness);
            mDrawInterface.DrawLine({}, {0.0f, 2.0f, 0.0f}, {0.3f, 0.9f, 0.4f}, true, mLineThickness);
            mDrawInterface.DrawLine({}, {0.0f, 0.0f, 2.0f}, {0.3f, 0.55f, 1.0f}, true, mLineThickness);
        }

        if (World->GetPersistentLevel() != nullptr) {
            const Core::TArray<CoreUObject::FObjectHandle> SelectedActors{mContext->GetSelectedActors()};
            const auto* SelectedComponent{CoreUObject::Cast<Engine::UActorComponent>(mContext->GetSelectedObject())};

            for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
                const bool Highlight{std::find(SelectedActors.begin(), SelectedActors.end(), Actor->GetHandle()) != SelectedActors.end()};

                if (Highlight && Actor->GetRootComponent() != nullptr) {
                    const auto* SelectedSceneComponent{SelectedComponent != nullptr && SelectedComponent->GetOwner() == Actor.get() ? CoreUObject::Cast<Engine::USceneComponent>(SelectedComponent) : nullptr};
                    Core::FVector Minimum{SelectedSceneComponent != nullptr ? SelectedSceneComponent->GetTransform().mLocation : Actor->GetRootComponent()->GetTransform().mLocation};
                    Core::FVector Maximum{Minimum};
                    bool HasBounds{};

                    for (const auto& Component : Actor->GetComponents()) {
                        const auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(Component.get())};

                        if (Primitive == nullptr || (SelectedSceneComponent != nullptr && Primitive != SelectedSceneComponent)) {
                            continue;
                        }

                        const Core::FBox Bounds{CalcComponentBounds(Primitive, View)};

                        if (!HasBounds) {
                            Minimum = Bounds.mMinimum;
                            Maximum = Bounds.mMaximum;
                            HasBounds = true;
                        } else {
                            Minimum = {std::min(Minimum.x, Bounds.mMinimum.x), std::min(Minimum.y, Bounds.mMinimum.y), std::min(Minimum.z, Bounds.mMinimum.z)};
                            Maximum = {std::max(Maximum.x, Bounds.mMaximum.x), std::max(Maximum.y, Bounds.mMaximum.y), std::max(Maximum.z, Bounds.mMaximum.z)};
                        }
                    }

                    const Core::FVector ScreenUp{View.mView(0, 1), View.mView(1, 1), View.mView(2, 1)};
                    const Core::FVector Extent{(Maximum - Minimum) * 0.5f};
                    const float Height{std::abs(ScreenUp.x) * Extent.x + std::abs(ScreenUp.y) * Extent.y + std::abs(ScreenUp.z) * Extent.z};
                    const Core::FVector Center{(Minimum + Maximum) * 0.5f};
                    const Core::FVector4 Clip{Core::FVector4{Center, 1.0f} * View.mViewProjection};
                    const float Margin{Clip.w > 0.0f ? 32.0f * Clip.w / (std::max(View.mSize.y, 1.0f) * View.mProjection(1, 1)) : 0.0f};
                    const Core::FString& Name{SelectedComponent != nullptr && SelectedComponent->GetOwner() == Actor.get() ? SelectedComponent->GetName() : Actor->GetName()};

                    mDrawInterface.DrawString({Center + ScreenUp * (Height + Margin), Name, {1.0f, 0.9f, 0.55f, 1.0f}, 18.0f, {1.0f, 1.0f}, true, false});
                }

                for (const auto& Component : Actor->GetComponents()) {
                    const auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(Component.get())};

                    if (Primitive != nullptr && Primitive->ShouldCreateRenderState() && (mShowFlags.mBounds || Highlight)) {
                        const bool DepthTest{!Highlight || CoreUObject::Cast<Engine::UBillboardComponent>(Primitive) != nullptr || CoreUObject::Cast<Engine::UBillboardTextComponent>(Primitive) != nullptr};
                        mDrawInterface.DrawBox(CalcComponentBounds(Primitive, View), Highlight ? Core::FVector{1.0f, 0.65f, 0.15f} : Core::FVector{0.25f, 0.8f, 0.95f}, DepthTest, mLineThickness);
                    }

                    if (const auto* Light{CoreUObject::Cast<Engine::ULightComponentBase>(Component.get())}; Light != nullptr && mShowFlags.mLightVisualizers) {
                        Light->DrawVisualization(mDrawInterface, Highlight);
                    }

                    if (const auto* Fog{CoreUObject::Cast<Engine::UHeightFogComponent>(Component.get())}; Fog != nullptr && Highlight) {
                        const Core::FVector Location{Fog->GetTransform().mLocation};
                        mDrawInterface.DrawBox(Core::FBox{Location - Core::FVector{0.5f, 0.5f, 0.5f}, Location + Core::FVector{0.5f, 0.5f, 0.5f}}, {1.0f, 0.65f, 0.15f}, true, mLineThickness);
                    }

                    const auto* Shape{CoreUObject::Cast<Engine::UShapeComponent>(Component.get())};

                    if (Shape != nullptr && (!Shape->ShouldDrawOnlyIfSelected() || Highlight)) {
                        Shape->DrawDebug(mDrawInterface, Highlight);
                    }
                }
            }
        }

        mGizmo.Draw(View, mDrawInterface);

        const Engine::FSceneViewFamily Family{World->GetScene(), Viewport->GetRenderTarget(), View, &mDrawInterface, mViewMode, mShowFlags};
        mContext->GetRenderer()->BeginRenderingViewFamily(Family);
    }

    /**
     * @brief 현재 카메라와 뷰포트 크기로 뷰·투영 행렬을 계산합니다.
     */
    Engine::FSceneView Editor::FEditorViewportClient::CalcSceneView(const Core::FVector2& Size) const {
        const Core::FQuat Rotation{mCameraRotation.Quaternion()};

        Engine::FSceneView View{};
        View.mLocation = mCameraLocation;
        View.mForward = Rotation.ForwardVector();
        View.mSize = Size;
        View.mOrthographic = mViewType != Editor::EEditorViewportType::Perspective;
        View.mOrthographicWidth = mOrthographicWidth;
        View.mFieldOfView = mFieldOfView;

        View.mView = Core::FMatrix::LookTo(mCameraLocation, View.mForward, Rotation.UpVector());

        const float Aspect{std::max(Size.x, 1.0f) / std::max(Size.y, 1.0f)};
        View.mProjection = View.mOrthographic ? Core::FMatrix::Orthographic(mOrthographicWidth, mOrthographicWidth / Aspect, 0.05f, 10000.0f) : Core::FMatrix::Perspective(mFieldOfView, Aspect, mNearClip, mFarClip);
        View.mViewProjection = View.mView * View.mProjection;

        return View;
    }

    /**
     * @brief 에디터 카메라를 초기 위치와 회전으로 되돌립니다.
     */
    void Editor::FEditorViewportClient::ResetCamera() {
        mCameraLocation = {-7.0f, -9.0f, 6.0f};
        mCameraRotation = {-26.0f, 50.0f, 0.0f};

        mOrbitPivot = {};
        mCameraSpeed = 4.0f;
        mPerspectiveLocation = mCameraLocation;
        mPerspectiveRotation = mCameraRotation;
        mPerspectivePivot = {};
        mOrthographicWidth = 20.0f;
        mFieldOfView = 60.0f;
        mNearClip = 0.05f;
        mFarClip = 10000.0f;

        if (mViewType != Editor::EEditorViewportType::Perspective) {
            const Editor::EEditorViewportType Type{mViewType};

            mViewType = Editor::EEditorViewportType::Perspective;
            SetViewType(Type);
        }
    }

    /**
     * @brief 유효한 위치와 회전으로 에디터 카메라를 설정합니다.
     */
    void Editor::FEditorViewportClient::SetCamera(const Core::FVector& Location, const Core::FRotator& Rotation) {
        SetViewType(Editor::EEditorViewportType::Perspective);
        mCameraLocation = Location;
        mCameraRotation = Rotation;
        mOrbitPivot = Location + Rotation.Vector() * 10.0f;
    }

    void Editor::FEditorViewportClient::SetViewType(Editor::EEditorViewportType Type) {
        if (Type < Editor::EEditorViewportType::Perspective || Type > Editor::EEditorViewportType::Right || mViewType == Type || mGizmo.IsDragging()) {
            return;
        }

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
            mCameraRotation = {-90.0f, 0.0f, 0.0f};
            break;
        case Editor::EEditorViewportType::Bottom:
            mCameraRotation = {90.0f, 0.0f, 0.0f};
            break;
        case Editor::EEditorViewportType::Front:
            mCameraRotation = {0.0f, 180.0f, 0.0f};
            break;
        case Editor::EEditorViewportType::Back:
            mCameraRotation = {};
            break;
        case Editor::EEditorViewportType::Left:
            mCameraRotation = {0.0f, -90.0f, 0.0f};
            break;
        case Editor::EEditorViewportType::Right:
            mCameraRotation = {0.0f, 90.0f, 0.0f};
            break;
        default:
            break;
        }

        mCameraLocation = mOrbitPivot - mCameraRotation.Vector() * 5000.0f;
    }

    Editor::EEditorViewportType Editor::FEditorViewportClient::GetViewType() const {
        return mViewType;
    }

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
            return "Perspective";
        }
    }

    Core::FVector Editor::FEditorViewportClient::GetPlacementLocation() const {
        return mViewType == Editor::EEditorViewportType::Perspective ? mCameraLocation + mCameraRotation.Vector() * 6.0f : mOrbitPivot;
    }

    /**
     * @brief 에디터 카메라의 월드 위치를 반환합니다.
     */
    const Core::FVector& Editor::FEditorViewportClient::GetCameraLocation() const {
        return mCameraLocation;
    }

    const Core::FRotator& Editor::FEditorViewportClient::GetCameraRotation() const {
        return mCameraRotation;
    }

    Engine::FEditorCameraState Editor::FEditorViewportClient::GetPerspectiveCamera() const {
        Engine::FEditorCameraState State{};
        State.Set(mViewType == Editor::EEditorViewportType::Perspective ? mCameraLocation : mPerspectiveLocation, mViewType == Editor::EEditorViewportType::Perspective ? mCameraRotation : mPerspectiveRotation, mFieldOfView, mNearClip, mFarClip);
        return State;
    }

    bool Editor::FEditorViewportClient::SetPerspectiveCamera(const Engine::FEditorCameraState& State) {
        if (!State.IsValid()) {
            return false;
        }

        mPerspectiveLocation = State.GetLocation();
        mPerspectiveRotation = State.GetRotation();
        mPerspectivePivot = mPerspectiveLocation + mPerspectiveRotation.Vector() * 10.0f;
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

    const Editor::FGizmoSettings* Editor::FEditorViewportClient::GetGizmoSettings() const {
        return mGizmo.GetSettings();
    }

    bool Editor::FEditorViewportClient::CanEditGizmoSettings() const {
        return !mGizmo.IsDragging() && mContext->CanEditWorld();
    }

    bool Editor::FEditorViewportClient::SetGizmoSetting(Editor::EGizmoSetting Setting, float Value) {
        if (!CanEditGizmoSettings() || !std::isfinite(Value)) {
            return false;
        }

        Editor::FGizmoSettings Settings{*mGizmo.GetSettings()};

        switch (Setting) {
        case Editor::EGizmoSetting::TranslationStep:
        case Editor::EGizmoSetting::RotationStep:
        case Editor::EGizmoSetting::ScaleStep:
            if (Value <= 0.0f) {
                return false;
            }

            if (Setting == Editor::EGizmoSetting::TranslationStep) {
                Settings.mTranslationStep = Value;
            } else if (Setting == Editor::EGizmoSetting::RotationStep) {
                Settings.mRotationStep = Value;
            } else {
                Settings.mScaleStep = Value;
            }

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
    Engine::FHitProxy Editor::FEditorViewportClient::GetHitProxy(const Engine::FSceneViewport* Viewport, const Core::FVector2& Position) const {
        Engine::UWorld* World{mContext->GetWorld()};

        if (Viewport == nullptr || World == nullptr) {
            return {};
        }

        const Engine::FSceneView View{CalcSceneView(Viewport->GetSize())};
        const Engine::FHitProxy GizmoHit{mGizmo.HitTest(Position, View)};

        if (GizmoHit.mType != Engine::EHitProxyType::None) {
            return GizmoHit;
        }

        Core::FRay Ray{};

        if (!View.Deproject(Position, Ray)) {
            return {};
        }

        Engine::FHitProxy Result{};
        float Closest{std::numeric_limits<float>::max()};

        if (World->GetPersistentLevel() == nullptr || mContext->GetRenderer() == nullptr) {
            return Result;
        }

        const Core::FVector Right{View.mView(0, 0), View.mView(1, 0), View.mView(2, 0)};
        const Core::FVector Up{View.mView(0, 1), View.mView(1, 1), View.mView(2, 1)};

        for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
            for (const auto& Component : Actor->GetComponents()) {
                if (const auto* Light{CoreUObject::Cast<Engine::ULightComponentBase>(Component.get())}; Light != nullptr && mShowFlags.mLightIcons && !Actor->IsPendingDestroy()) {
                    const Engine::FPrimitiveSceneProxy Proxy{Light->CreateLightProxy().CreateIconProxy()};

                    if (Engine::RaycastMesh(&Engine::GetPrimitiveMesh(Engine::EPrimitiveType::Plane), Engine::MakeSpriteWorld(Proxy.mWorld, *Proxy.mSprite, View), Ray, Closest)) {
                        Result = {Engine::EHitProxyType::Object, Light->GetHandle()};
                    }
                }

                if (const auto* Fog{CoreUObject::Cast<Engine::UHeightFogComponent>(Component.get())}; Fog != nullptr && mShowFlags.mFogIcons && !Actor->IsPendingDestroy()) {
                    const Engine::FPrimitiveSceneProxy Proxy{Fog->CreateFogProxy().CreateIconProxy()};
                    if (Engine::RaycastMesh(&Engine::GetPrimitiveMesh(Engine::EPrimitiveType::Plane), Engine::MakeSpriteWorld(Proxy.mWorld, *Proxy.mSprite, View), Ray, Closest)) {
                        Result = {Engine::EHitProxyType::Object, Fog->GetHandle()};
                    }
                }

                const auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(Component.get())};
                const auto* Sprite{CoreUObject::Cast<Engine::UBillboardComponent>(Component.get())};
                const auto* Text{CoreUObject::Cast<Engine::UBillboardTextComponent>(Component.get())};

                if (!mShowFlags.mPrimitives || Actor->IsPendingDestroy() || Primitive == nullptr || !Primitive->ShouldCreateRenderState()) {
                    continue;
                }

                if (Sprite != nullptr) {
                    const Engine::FPrimitiveSceneProxy Proxy{Sprite->CreateSceneProxy()};

                    if (mShowFlags.mSprites && Engine::RaycastMesh(&Engine::GetPrimitiveMesh(Engine::EPrimitiveType::Plane), Engine::MakeSpriteWorld(Proxy.mWorld, *Proxy.mSprite, View), Ray, Closest)) {
                        Result = {Engine::EHitProxyType::Object, Sprite->GetHandle()};
                    }

                    continue;
                }

                if (Text == nullptr) {
                    if (Engine::RaycastMesh(&Primitive->GetMeshData(), Primitive->GetWorldMatrix(), Ray, Closest)) {
                        Result = {Engine::EHitProxyType::Object, Primitive->GetHandle()};
                    }

                    continue;
                }

                Core::FVector2 Minimum{};
                Core::FVector2 Maximum{};

                if (!mShowFlags.mText || !mContext->GetRenderer()->GetTextBounds(Text->GetText(), Minimum, Maximum)) {
                    continue;
                }

                const Core::FTransform& Transform{Text->GetTransform()};
                const Core::FVector4 Clip{Core::FVector4{Transform.mLocation, 1.0f} * View.mViewProjection};
                Core::FVector Intersection{};

                if (Clip.w <= 0.0f || Clip.z < 0.0f || Clip.z > Clip.w || !Core::Math::IntersectPlane(Ray, Transform.mLocation, View.mForward, Intersection)) {
                    continue;
                }

                const float Distance{(Intersection - Ray.mOrigin).Dot(Ray.mDirection)};
                const Core::FVector Offset{Intersection - Transform.mLocation};
                const float Horizontal{Offset.Dot(Right) / (Text->GetTextSize() * std::abs(Transform.mScale.y))};
                const float Vertical{Offset.Dot(Up) / (Text->GetTextSize() * std::abs(Transform.mScale.z))};

                if (Distance < Closest && Horizontal >= Minimum.x && Horizontal <= Maximum.x && Vertical >= Minimum.y && Vertical <= Maximum.y) {
                    Closest = Distance;
                    Result = {Engine::EHitProxyType::Object, Text->GetHandle()};
                }
            }
        }

        return Result;
    }

    Core::FBox Editor::FEditorViewportClient::CalcComponentBounds(const Engine::UPrimitiveComponent* Component, const Engine::FSceneView& View) const {
        if (const auto* Sprite{CoreUObject::Cast<Engine::UBillboardComponent>(Component)}) {
            const Engine::FPrimitiveSceneProxy Proxy{Sprite->CreateSceneProxy()};
            return Core::FBox{{-0.5f, -0.5f, -0.001f}, {0.5f, 0.5f, 0.001f}}.Transformed(Engine::MakeSpriteWorld(Proxy.mWorld, *Proxy.mSprite, View));
        }

        const auto* Text{CoreUObject::Cast<Engine::UBillboardTextComponent>(Component)};
        Core::FVector2 Minimum{};
        Core::FVector2 Maximum{};

        if (Text == nullptr || mContext->GetRenderer() == nullptr || !mContext->GetRenderer()->GetTextBounds(Text->GetText(), Minimum, Maximum)) {
            return Component->GetWorldBounds();
        }

        const Core::FTransform& Transform{Text->GetTransform()};
        const Core::FVector Right{View.mView(0, 0), View.mView(1, 0), View.mView(2, 0)};
        const Core::FVector Up{View.mView(0, 1), View.mView(1, 1), View.mView(2, 1)};
        const float WidthScale{Text->GetTextSize() * std::abs(Transform.mScale.y)};
        const float HeightScale{Text->GetTextSize() * std::abs(Transform.mScale.z)};
        const Core::FVector Center{Transform.mLocation + Right * ((Minimum.x + Maximum.x) * 0.5f * WidthScale) + Up * ((Minimum.y + Maximum.y) * 0.5f * HeightScale)};
        const float HalfWidth{(Maximum.x - Minimum.x) * 0.5f * WidthScale};
        const float HalfHeight{(Maximum.y - Minimum.y) * 0.5f * HeightScale};
        const Core::FVector Extent{std::abs(Right.x) * HalfWidth + std::abs(Up.x) * HalfHeight, std::abs(Right.y) * HalfWidth + std::abs(Up.y) * HalfHeight, std::abs(Right.z) * HalfWidth + std::abs(Up.z) * HalfHeight};

        return {Center - Extent, Center + Extent};
    }

    bool Editor::FEditorViewportClient::DropMesh(const std::filesystem::path& Path, const Core::FVector2& Position, const Core::FVector2& Size) {
        Engine::UWorld* World{mContext->GetWorld()};
        Engine::FProject* Project{mContext->GetProject()};

        if (!mContext->CanEditWorld() || mGizmo.IsDragging() || World == nullptr || Project == nullptr) {
            return false;
        }

        const Engine::FSceneView View{CalcSceneView(Size)};
        Core::FRay Ray{};

        if (!View.Deproject(Position, Ray)) {
            return false;
        }

        Engine::FWorldHit Hit{};
        Core::FVector Location{mViewType == Editor::EEditorViewportType::Perspective ? Ray.mOrigin + Ray.mDirection * 6.0f : mOrbitPivot};
        const bool OnSurface{World->Raycast(Ray, Hit)};
        const bool OnGround{!OnSurface && Core::Math::IntersectPlane(Ray, mViewType == Editor::EEditorViewportType::Perspective ? Core::FVector{} : mOrbitPivot, mViewType == Editor::EEditorViewportType::Perspective ? Core::FVector{0.0f, 0.0f, 1.0f} : View.mForward, Location)};

        if (OnSurface) {
            Location = Hit.mPosition;
        }

        Core::FString Error{};
        const Asset::UStaticMesh* Mesh{Project->GetAssets()->LoadMesh(Path, Error)};

        if (Mesh == nullptr) {
            mContext->AddLog(Error);
            return false;
        }

        if (OnSurface || (OnGround && mViewType == Editor::EEditorViewportType::Perspective)) {
            Location.z -= Mesh->GetBounds().mMinimum.z;
        }

        return mContext->SpawnMesh(Path, Core::FTransform{Location, {}, {1.0f, 1.0f, 1.0f}}) != nullptr;
    }

    Engine::EViewMode Editor::FEditorViewportClient::GetViewMode() const {
        return mViewMode;
    }

    void Editor::FEditorViewportClient::SetViewMode(Engine::EViewMode Mode) {
        if (Mode == Engine::EViewMode::Lit_Phong || Mode == Engine::EViewMode::Lit_Gouraud || Mode == Engine::EViewMode::Lit_Lambert || Mode == Engine::EViewMode::Unlit || Mode == Engine::EViewMode::WorldNormal || Mode == Engine::EViewMode::SceneDepth || Mode == Engine::EViewMode::Wireframe) {
            mViewMode = Mode;
            if (Mode != Engine::EViewMode::WorldNormal && Mode != Engine::EViewMode::SceneDepth) {
                mColorViewMode = Mode;
            }
        }
    }

    Engine::EViewMode Editor::FEditorViewportClient::GetColorViewMode() const {
        return mColorViewMode;
    }

    const Engine::FEngineShowFlags& Editor::FEditorViewportClient::GetShowFlags() const {
        return mShowFlags;
    }

    void Editor::FEditorViewportClient::SetShowFlags(const Engine::FEngineShowFlags& Flags) {
        mGizmo.Cancel();
        mShowFlags = Flags;
    }

    float Editor::FEditorViewportClient::GetLineThickness() const {
        return mLineThickness;
    }

    bool Editor::FEditorViewportClient::SetLineThickness(float Thickness) {
        if (!std::isfinite(Thickness) || Thickness < 0.5f || Thickness > 10.0f) {
            return false;
        }

        mLineThickness = Thickness;

        return true;
    }

    float Editor::FEditorViewportClient::GetGridSpacing() const {
        return mGridSpacing;
    }

    bool Editor::FEditorViewportClient::SetGridSpacing(float Spacing) {
        if (!std::isfinite(Spacing) || Spacing < 0.01f || Spacing > 1000.0f) {
            return false;
        }

        mGridSpacing = Spacing;

        return true;
    }

    float Editor::FEditorViewportClient::GetCameraSensitivity() const {
        return mCameraSensitivity;
    }

    bool Editor::FEditorViewportClient::SetCameraSensitivity(float Sensitivity) {
        if (!std::isfinite(Sensitivity) || Sensitivity < 0.01f || Sensitivity > 2.0f) {
            return false;
        }

        mCameraSensitivity = Sensitivity;

        return true;
    }

    float Editor::FEditorViewportClient::GetCameraSpeed() const {
        return mCameraSpeed;
    }

    bool Editor::FEditorViewportClient::SetCameraSpeed(float Speed) {
        if (!std::isfinite(Speed) || Speed < 0.1f || Speed > 128.0f) {
            return false;
        }

        mCameraSpeed = Speed;

        return true;
    }

    float Editor::FEditorViewportClient::GetOrthographicWidth() const {
        return mOrthographicWidth;
    }

    bool Editor::FEditorViewportClient::SetOrthographicWidth(float Width) {
        if (!std::isfinite(Width) || Width < 0.01f || Width > 100000.0f) {
            return false;
        }

        mOrthographicWidth = Width;
        return true;
    }

    bool Editor::FEditorViewportClient::DropActor(const Core::FString& Preset, const Core::FVector2& Position, const Core::FVector2& Size) {
        Engine::UWorld* World{mContext->GetWorld()};

        if (!mContext->CanEditWorld() || mGizmo.IsDragging() || World == nullptr) {
            return false;
        }

        const Engine::FSceneView View{CalcSceneView(Size)};
        Core::FRay Ray{};

        if (!View.Deproject(Position, Ray)) {
            return false;
        }

        Core::FVector Location{mViewType == Editor::EEditorViewportType::Perspective ? Ray.mOrigin + Ray.mDirection * 6.0f : mOrbitPivot};
        Engine::FWorldHit Hit{};

        if (World->Raycast(Ray, Hit)) {
            Location = Hit.mPosition;
        } else {
            Core::Math::IntersectPlane(Ray, mViewType == Editor::EEditorViewportType::Perspective ? Core::FVector{} : mOrbitPivot, mViewType == Editor::EEditorViewportType::Perspective ? Core::FVector{0.0f, 0.0f, 1.0f} : View.mForward, Location);
        }

        const bool Light{Preset.find("Light") != Core::FString::npos};
        const Core::FRotator Rotation{Preset.find("Directional") != Core::FString::npos || Preset.find("Spot") != Core::FString::npos ? Core::FRotator{-45.0f, 0.0f, 0.0f} : Core::FRotator{}};
        const Core::FVector Offset{0.0f, 0.0f, Light ? 1.0f : Preset == "Cube" || Preset == "Sphere" || Preset == "AStaticMeshActor" ? 0.5f : 0.0f};

        return mContext->SpawnActorPreset(Preset, Core::FTransform{Location + Offset, Rotation.Quaternion(), {1.0f, 1.0f, 1.0f}}) != nullptr;
    }

}
