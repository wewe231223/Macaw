/**
 * @file FEditorViewportClient.cpp
 * @brief 에디터 카메라, 선택, 공통 명령과 기즈모 입력을 처리합니다.
 * @ingroup Editor
 */

#include "Editor/FEditorViewportClient.h"
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
#include "Engine/UStaticMesh.h"

#include <algorithm>
#include <cmath>
#include <limits>

/**
 * @brief FEditorViewportClient의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FEditorViewportClient::FEditorViewportClient(FEditorContext* Context)
	: mContext{Context},
	  mGizmo{Context, this} {
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FEditorViewportClient::~FEditorViewportClient() = default;

void FEditorViewportClient::SetEditorContext(FEditorContext* Context) {
    if (Context == nullptr || Context == mContext) {
        return;
    }
    mGizmo.SetEditorContext(Context);
    mContext = Context;
}

/**
 * @brief 공통 실행 검사를 통과한 명령으로 변환 모드, 좌표계 또는 스냅 설정을 변경합니다.
 */
bool FEditorViewportClient::ExecuteCommand(EViewportCommand Command) {
    if (!CanExecuteCommand(Command)) {
        return false;
    }

    switch (Command) {
    case EViewportCommand::ToggleGrid:
        mShowFlags.mGrid = !mShowFlags.mGrid;
        return true;
    case EViewportCommand::Select:
        SetGizmoMode(EGizmoMode::None);
        break;
    case EViewportCommand::Translate:
        SetGizmoMode(EGizmoMode::Translate);
        break;
    case EViewportCommand::Rotate:
        SetGizmoMode(EGizmoMode::Rotate);
        break;
    case EViewportCommand::Scale:
        SetGizmoMode(EGizmoMode::Scale);
        break;
    case EViewportCommand::WorldSpace:
        SetGizmoSpace(EGizmoSpace::World);
        break;
    case EViewportCommand::LocalSpace:
        SetGizmoSpace(EGizmoSpace::Local);
        break;
    case EViewportCommand::ToggleCoordinateSystem:
        SetGizmoSpace(mGizmoSpace == EGizmoSpace::World ? EGizmoSpace::Local : EGizmoSpace::World);
        break;
    case EViewportCommand::CycleTransformMode:
        SetGizmoMode(mGizmoMode == EGizmoMode::Translate ? EGizmoMode::Rotate : mGizmoMode == EGizmoMode::Rotate ? EGizmoMode::Scale : EGizmoMode::Translate);
        break;
    case EViewportCommand::ToggleTranslationSnap:
    case EViewportCommand::ToggleRotationSnap:
    case EViewportCommand::ToggleScaleSnap:
    case EViewportCommand::ToggleTransformSnap:
        return ToggleGizmoSetting(Command);
    default:
        return false;
    }

    return true;
}

/**
 * @brief 드래그 잠금, 스케일 좌표계 제한과 명령 지원 여부를 검사합니다.
 */
bool FEditorViewportClient::CanExecuteCommand(EViewportCommand Command) const {
    if (Command == EViewportCommand::ToggleGrid) {
        return true;
    }
    if (!CanEditGizmoSettings()) {
        return false;
    }

    switch (Command) {
    case EViewportCommand::Select:
    case EViewportCommand::Translate:
    case EViewportCommand::Rotate:
    case EViewportCommand::Scale:
    case EViewportCommand::LocalSpace:
    case EViewportCommand::CycleTransformMode:
    case EViewportCommand::ToggleTranslationSnap:
    case EViewportCommand::ToggleRotationSnap:
    case EViewportCommand::ToggleScaleSnap:
        return true;
    case EViewportCommand::ToggleTransformSnap:
        return mGizmoMode != EGizmoMode::None;
    case EViewportCommand::WorldSpace:
    case EViewportCommand::ToggleCoordinateSystem:
        return mGizmoMode != EGizmoMode::Scale;
    default:
        return false;
    }
}

/**
 * @brief 실행 가능 여부와 별개로 현재 변환 모드와 좌표계의 선택 상태를 조회합니다.
 */
bool FEditorViewportClient::IsCommandChecked(EViewportCommand Command) const {
    switch (Command) {
    case EViewportCommand::ToggleGrid:
        return mShowFlags.mGrid;
    case EViewportCommand::Select:
        return mGizmoMode == EGizmoMode::None;
    case EViewportCommand::Translate:
        return mGizmoMode == EGizmoMode::Translate;
    case EViewportCommand::Rotate:
        return mGizmoMode == EGizmoMode::Rotate;
    case EViewportCommand::Scale:
        return mGizmoMode == EGizmoMode::Scale;
    case EViewportCommand::WorldSpace:
        return mGizmoSpace == EGizmoSpace::World;
    case EViewportCommand::LocalSpace:
    case EViewportCommand::ToggleCoordinateSystem:
        return mGizmoSpace == EGizmoSpace::Local;
    case EViewportCommand::ToggleTranslationSnap:
        return mGizmo.GetSettings()->mTranslationSnap;
    case EViewportCommand::ToggleRotationSnap:
        return mGizmo.GetSettings()->mRotationSnap;
    case EViewportCommand::ToggleScaleSnap:
        return mGizmo.GetSettings()->mScaleSnap;
    case EViewportCommand::ToggleTransformSnap:
        return mGizmoMode == EGizmoMode::Translate ? IsCommandChecked(EViewportCommand::ToggleTranslationSnap) : mGizmoMode == EGizmoMode::Rotate ? IsCommandChecked(EViewportCommand::ToggleRotationSnap) : mGizmoMode == EGizmoMode::Scale && IsCommandChecked(EViewportCommand::ToggleScaleSnap);
    default:
        return false;
    }
}

/**
 * @brief 현재 프레임의 뷰포트 입력을 처리합니다.
 */
void FEditorViewportClient::Input(FSceneViewport* Viewport, const FViewportInput& Input, float DeltaTime) {
    if (Viewport == nullptr) {
        return;
    }
    const FVector2 Size{Viewport->GetSize()};
    if (!mGizmo.IsDragging() && Input.mFocused && !Input.mRightDown && !Input.mAlt && !Input.mControl) {
        bool ModeRequested{};
        for (const FViewportCommandInfo& Info : FViewportCommandInfo::GetCommands()) {
            if (Info.mInput == nullptr || !(Input.*Info.mInput)) {
                continue;
            }
            const bool ModeCommand{Info.mTransformMode || Info.mCommand == EViewportCommand::CycleTransformMode};
            if ((ModeCommand && ModeRequested) || (Info.mCommand == EViewportCommand::CycleTransformMode && Input.mLeftDown)) {
                continue;
            }
            ModeRequested = ModeRequested || ModeCommand;
            ExecuteCommand(Info.mCommand);
        }
    }
    const FSceneView View{CalcSceneView(Size)};
    const FHitProxy Hit{!mGizmo.IsDragging() && Input.mHovered ? Viewport->GetHitProxy(Input.mMouse) : FHitProxy{}};
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
        if (Hit.mType == EHitProxyType::Object) {
            UObject* Object{mContext->ResolveObject(Hit.mObject)};
            const auto* Component{Cast<UActorComponent>(Object)};
            AActor* Actor{Component != nullptr ? Component->GetOwner() : Cast<AActor>(Object)};
            if (Actor != nullptr) {
                if (Input.mControl) {
                    mContext->ToggleActorSelection(Actor->GetHandle());
                } else {
                    mContext->SelectActor(Actor->GetHandle());
                }
            }
        } else if (Hit.mType == EHitProxyType::None && !Input.mControl) {
            mContext->ClearSelection();
        }
    }
}

/**
 * @brief 뷰포트 입력으로 카메라 이동, 회전, 팬 또는 오빗을 적용합니다.
 * @note 자유 시점 조작은 피벗까지의 거리를 유지하며 갱신한 카메라의 시선 앞에 피벗을 배치합니다.
 */
bool FEditorViewportClient::NavigateCamera(const FViewportInput& Input, const FVector2& Size, float DeltaTime) {
    if (!Input.mHovered && !Input.mFocused) {
        return false;
    }

    if (Input.mFocusSelection && Input.mFocused && !Input.mRightDown && !Input.mAlt) {
        FocusSelection(Size);
    }

    const FQuat Rotation{mCameraRotation.Quaternion()};

    if (mViewType != EEditorViewportType::Perspective) {
        if (Input.mWheel != 0.0f && Input.mHovered) {
            const float PreviousWidth{mOrthographicWidth};
            mOrthographicWidth = std::clamp(mOrthographicWidth * std::pow(0.85f, Input.mWheel), 0.1f, 10000.0f);
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
            SetCameraSpeed(std::clamp(mCameraSpeed * std::pow(1.25f, Input.mWheel), 0.1f, 128.0f));
        } else {
            mCameraLocation += Rotation.ForwardVector() * Input.mWheel * mCameraSpeed * 0.5f;
        }
    }

    if (Input.mMiddleDown || (Input.mLeftDown && Input.mRightDown && !Input.mAlt)) {
        const float Distance{std::max(mCameraLocation.Distance(mOrbitPivot), 1.0f)};
        const float PixelScale{2.0f * Distance * std::tan(Math::ToRadians(30.0f)) / std::max(Size.y, 1.0f)};
        const FVector Pan{Rotation * FVector{0.0f, -Input.mDelta.x, Input.mDelta.y} * PixelScale};
        mCameraLocation += Pan;
        mOrbitPivot += Pan;

        return true;
    }

    if (Input.mAlt && (Input.mLeftDown || Input.mRightDown)) {
        float Distance{std::max(mCameraLocation.Distance(mOrbitPivot), 0.1f)};
        FVector Forward{mOrbitPivot - mCameraLocation};

        if (Forward.Normalize()) {
            const FRotator Facing{Forward.Rotation()};
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

        const FQuat FlightRotation{mCameraRotation.Quaternion()};
        const FVector LocalMovement{static_cast<float>(Input.mForward) - static_cast<float>(Input.mBackward), static_cast<float>(Input.mRight) - static_cast<float>(Input.mLeft), 0.0f};
        FVector Movement{FlightRotation * LocalMovement};
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
void FEditorViewportClient::FocusSelection(const FVector2& Size) {
    const AActor* Actor{mContext->GetSelectedActor()};
    const auto* Component{Actor != nullptr ? Cast<UPrimitiveComponent>(Actor->GetRootComponent()) : nullptr};

    if (Component == nullptr) {
        return;
    }

    const FBox Bounds{CalcComponentBounds(Component, CalcSceneView(Size))};
    mOrbitPivot = Bounds.mMinimum.Lerp(Bounds.mMaximum, 0.5f);

    const float Radius{Bounds.mMinimum.Distance(Bounds.mMaximum) * 0.5f};
    if (mViewType != EEditorViewportType::Perspective) {
        const float Aspect{std::max(Size.x, 1.0f) / std::max(Size.y, 1.0f)};
        mOrthographicWidth = std::clamp(Radius * 2.4f * std::max(Aspect, 1.0f), 0.1f, 10000.0f);
        mCameraLocation = mOrbitPivot - mCameraRotation.Vector() * 5000.0f;
        return;
    }
    const float HalfFieldOfView{std::atan(std::tan(Math::ToRadians(30.0f)) * std::min(std::max(Size.x, 1.0f) / std::max(Size.y, 1.0f), 1.0f))};
    const float Distance{std::max(0.5f, Radius / std::sin(HalfFieldOfView) * 1.2f)};

    mCameraLocation = mOrbitPivot - mCameraRotation.Vector() * Distance;
}

/**
 * @brief 월드 원점에 고정한 그리드와 에디터 보조 요소를 포함한 뷰포트를 그립니다.
 */
void FEditorViewportClient::Draw(FSceneViewport* Viewport) {
    UWorld* World{mContext->GetWorld()};

    if (World == nullptr || World->GetScene() == nullptr || mContext->GetRenderer() == nullptr) {
        return;
    }

    const FSceneView View{CalcSceneView(Viewport->GetSize())};

    mDrawInterface.Clear();

    if (mShowFlags.mGrid) {
        const float Spacing{mGridSpacing};
        const float Extent{50.0f * Spacing};
        for (Int32 Index{-50}; Index <= 50; ++Index) {
            const float Offset{Index * Spacing};
            const FVector Color{Index % 10 == 0 ? FVector{0.22f, 0.25f, 0.30f} : FVector{0.13f, 0.15f, 0.19f}};
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
        const TArray<FObjectHandle> SelectedActors{mContext->GetSelectedActors()};
        const auto* SelectedComponent{Cast<UActorComponent>(mContext->GetSelectedObject())};
        for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
            const bool Highlight{std::find(SelectedActors.begin(), SelectedActors.end(), Actor->GetHandle()) != SelectedActors.end()};
            if (Highlight && Actor->GetRootComponent() != nullptr) {
                const auto* SelectedSceneComponent{SelectedComponent != nullptr && SelectedComponent->GetOwner() == Actor.get() ? Cast<USceneComponent>(SelectedComponent) : nullptr};
                FVector Minimum{SelectedSceneComponent != nullptr ? SelectedSceneComponent->GetTransform().mLocation : Actor->GetRootComponent()->GetTransform().mLocation};
                FVector Maximum{Minimum};
                bool HasBounds{};
                for (const auto& Component : Actor->GetComponents()) {
                    const auto* Primitive{Cast<UPrimitiveComponent>(Component.get())};
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
                const FString& Name{SelectedComponent != nullptr && SelectedComponent->GetOwner() == Actor.get() ? SelectedComponent->GetName() : Actor->GetName()};
                mDrawInterface.DrawString({Center + ScreenUp * (Height + Margin), Name, {1.0f, 0.9f, 0.55f, 1.0f}, 18.0f, {1.0f, 1.0f}, true, false});
            }
            for (const auto& Component : Actor->GetComponents()) {
                const auto* Primitive{Cast<UPrimitiveComponent>(Component.get())};
                if (Primitive != nullptr && Primitive->ShouldCreateRenderState() && (mShowFlags.mBounds || Highlight)) {
                    const bool DepthTest{!Highlight || Cast<UBillboardComponent>(Primitive) != nullptr || Cast<UBillboardTextComponent>(Primitive) != nullptr};
                    mDrawInterface.DrawBox(CalcComponentBounds(Primitive, View), Highlight ? FVector{1.0f, 0.65f, 0.15f} : FVector{0.25f, 0.8f, 0.95f}, DepthTest, mLineThickness);
                }
                const auto* Shape{Cast<UShapeComponent>(Component.get())};
                if (Shape != nullptr && (!Shape->ShouldDrawOnlyIfSelected() || Highlight)) {
                    Shape->DrawDebug(mDrawInterface, Highlight);
                }
            }
        }
    }

    mGizmo.Draw(View, mDrawInterface);

    const FSceneViewFamily Family{World->GetScene(), Viewport->GetRenderTarget(), View, &mDrawInterface, mViewMode, mShowFlags};
    mContext->GetRenderer()->BeginRenderingViewFamily(Family);
}

/**
 * @brief 현재 카메라와 뷰포트 크기로 뷰·투영 행렬을 계산합니다.
 */
FSceneView FEditorViewportClient::CalcSceneView(const FVector2& Size) const {
    const FQuat Rotation{mCameraRotation.Quaternion()};

    FSceneView View{};
    View.mLocation = mCameraLocation;
    View.mForward = Rotation.ForwardVector();
    View.mSize = Size;
    View.mOrthographic = mViewType != EEditorViewportType::Perspective;
    View.mOrthographicWidth = mOrthographicWidth;

    View.mView = FMatrix::LookTo(mCameraLocation, View.mForward, Rotation.UpVector());

    const float Aspect{std::max(Size.x, 1.0f) / std::max(Size.y, 1.0f)};
    View.mProjection = View.mOrthographic ? FMatrix::Orthographic(mOrthographicWidth, mOrthographicWidth / Aspect, 0.05f, 10000.0f) : FMatrix::Perspective(View.mFieldOfView, Aspect, 0.05f, 10000.0f);
    View.mViewProjection = View.mView * View.mProjection;

    return View;
}

/**
 * @brief 에디터 카메라를 초기 위치와 회전으로 되돌립니다.
 */
void FEditorViewportClient::ResetCamera() {
    mCameraLocation = {-7.0f, -9.0f, 6.0f};
    mCameraRotation = {-26.0f, 50.0f, 0.0f};

    mOrbitPivot = {};
    mCameraSpeed = 4.0f;
    mPerspectiveLocation = mCameraLocation;
    mPerspectiveRotation = mCameraRotation;
    mPerspectivePivot = {};
    mOrthographicWidth = 20.0f;
    if (mViewType != EEditorViewportType::Perspective) {
        const EEditorViewportType Type{mViewType};
        mViewType = EEditorViewportType::Perspective;
        SetViewType(Type);
    }
}

/**
 * @brief 유효한 위치와 회전으로 에디터 카메라를 설정합니다.
 */
void FEditorViewportClient::SetCamera(const FVector& Location, const FRotator& Rotation) {
    SetViewType(EEditorViewportType::Perspective);
    mCameraLocation = Location;
    mCameraRotation = Rotation;
    mOrbitPivot = Location + Rotation.Vector() * 10.0f;
}

void FEditorViewportClient::SetViewType(EEditorViewportType Type) {
    if (Type < EEditorViewportType::Perspective || Type > EEditorViewportType::Right || mViewType == Type || mGizmo.IsDragging()) {
        return;
    }
    if (mViewType == EEditorViewportType::Perspective) {
        mPerspectiveLocation = mCameraLocation;
        mPerspectiveRotation = mCameraRotation;
        mPerspectivePivot = mOrbitPivot;
    }
    mViewType = Type;
    if (Type == EEditorViewportType::Perspective) {
        mCameraLocation = mPerspectiveLocation;
        mCameraRotation = mPerspectiveRotation;
        mOrbitPivot = mPerspectivePivot;
        return;
    }
    switch (Type) {
    case EEditorViewportType::Top:
        mCameraRotation = {-90.0f, 0.0f, 0.0f};
        break;
    case EEditorViewportType::Bottom:
        mCameraRotation = {90.0f, 0.0f, 0.0f};
        break;
    case EEditorViewportType::Front:
        mCameraRotation = {0.0f, 180.0f, 0.0f};
        break;
    case EEditorViewportType::Back:
        mCameraRotation = {};
        break;
    case EEditorViewportType::Left:
        mCameraRotation = {0.0f, -90.0f, 0.0f};
        break;
    case EEditorViewportType::Right:
        mCameraRotation = {0.0f, 90.0f, 0.0f};
        break;
    default:
        break;
    }
    mCameraLocation = mOrbitPivot - mCameraRotation.Vector() * 5000.0f;
}

EEditorViewportType FEditorViewportClient::GetViewType() const {
    return mViewType;
}

const char* FEditorViewportClient::GetViewTypeName() const {
    switch (mViewType) {
    case EEditorViewportType::Top:
        return "Top";
    case EEditorViewportType::Bottom:
        return "Bottom";
    case EEditorViewportType::Front:
        return "Front";
    case EEditorViewportType::Back:
        return "Back";
    case EEditorViewportType::Left:
        return "Left";
    case EEditorViewportType::Right:
        return "Right";
    default:
        return "Perspective";
    }
}

FVector FEditorViewportClient::GetPlacementLocation() const {
    return mViewType == EEditorViewportType::Perspective ? mCameraLocation + mCameraRotation.Vector() * 6.0f : mOrbitPivot;
}

/**
 * @brief 에디터 카메라의 월드 위치를 반환합니다.
 */
const FVector& FEditorViewportClient::GetCameraLocation() const {
    return mCameraLocation;
}

const FRotator& FEditorViewportClient::GetCameraRotation() const {
    return mCameraRotation;
}

/**
 * @brief 뷰포트 변환 편집 기즈모를 반환합니다.
 */
FTransformGizmo& FEditorViewportClient::GetGizmo() {
    return mGizmo;
}

const FGizmoSettings* FEditorViewportClient::GetGizmoSettings() const {
    return mGizmo.GetSettings();
}

bool FEditorViewportClient::CanEditGizmoSettings() const {
    return !mGizmo.IsDragging() && mContext->CanEditWorld();
}

bool FEditorViewportClient::SetGizmoSetting(EGizmoSetting Setting, float Value) {
    if (!CanEditGizmoSettings() || !std::isfinite(Value)) {
        return false;
    }
    FGizmoSettings Settings{*mGizmo.GetSettings()};
    switch (Setting) {
    case EGizmoSetting::TranslationStep:
    case EGizmoSetting::RotationStep:
    case EGizmoSetting::ScaleStep:
        if (Value <= 0.0f) {
            return false;
        }
        if (Setting == EGizmoSetting::TranslationStep) {
            Settings.mTranslationStep = Value;
        } else if (Setting == EGizmoSetting::RotationStep) {
            Settings.mRotationStep = Value;
        } else {
            Settings.mScaleStep = Value;
        }
        break;
    case EGizmoSetting::Size:
        Settings.mSize = std::clamp(Value, 0.5f, 2.0f);
        break;
    default:
        return false;
    }
    mGizmo.SetSettings(Settings);
    return true;
}

bool FEditorViewportClient::ToggleGizmoSetting(EViewportCommand Command) {
    FGizmoSettings Settings{*mGizmo.GetSettings()};
    switch (Command) {
    case EViewportCommand::ToggleTranslationSnap:
        Settings.mTranslationSnap = !Settings.mTranslationSnap;
        break;
    case EViewportCommand::ToggleRotationSnap:
        Settings.mRotationSnap = !Settings.mRotationSnap;
        break;
    case EViewportCommand::ToggleScaleSnap:
        Settings.mScaleSnap = !Settings.mScaleSnap;
        break;
    case EViewportCommand::ToggleTransformSnap:
        return mGizmoMode == EGizmoMode::Translate ? ToggleGizmoSetting(EViewportCommand::ToggleTranslationSnap) : mGizmoMode == EGizmoMode::Rotate ? ToggleGizmoSetting(EViewportCommand::ToggleRotationSnap) : mGizmoMode == EGizmoMode::Scale && ToggleGizmoSetting(EViewportCommand::ToggleScaleSnap);
    default:
        return false;
    }
    mGizmo.SetSettings(Settings);
    return true;
}

/**
 * @brief 변환 모드를 저장하고 스케일을 선택하면 로컬 좌표계를 적용합니다.
 */
void FEditorViewportClient::SetGizmoMode(EGizmoMode Mode) {
    mGizmoMode = Mode;

    if (mGizmoMode == EGizmoMode::Scale) {
        mGizmoSpace = EGizmoSpace::Local;
    }
}

/**
 * @brief 클라이언트가 관리하는 변환 기즈모 모드를 반환합니다.
 */
EGizmoMode FEditorViewportClient::GetGizmoMode() const {
    return mGizmoMode;
}

/**
 * @brief 명령 실행 경로에서 검증한 기즈모 좌표계를 저장합니다.
 */
void FEditorViewportClient::SetGizmoSpace(EGizmoSpace Space) {
    mGizmoSpace = Space;
}

/**
 * @brief 클라이언트가 관리하는 기즈모 축의 좌표 공간을 반환합니다.
 */
EGizmoSpace FEditorViewportClient::GetGizmoSpace() const {
    return mGizmoSpace;
}

/**
 * @brief 기즈모 메시를 우선 검사하고 공통 CPU 광선 질의로 메시 컴포넌트의 핸들을 구합니다.
 */
FHitProxy FEditorViewportClient::GetHitProxy(const FSceneViewport* Viewport, const FVector2& Position) const {
    UWorld* World{mContext->GetWorld()};

    if (Viewport == nullptr || World == nullptr) {
        return {};
    }

    const FSceneView View{CalcSceneView(Viewport->GetSize())};
    const FHitProxy GizmoHit{mGizmo.HitTest(Position, View)};

    if (GizmoHit.mType != EHitProxyType::None) {
        return GizmoHit;
    }

    FRay Ray{};

    if (!View.Deproject(Position, Ray)) {
        return {};
    }
    FHitProxy Result{};
    float Closest{std::numeric_limits<float>::max()};
    if (World->GetPersistentLevel() == nullptr || mContext->GetRenderer() == nullptr) {
        return Result;
    }
    const FVector Right{View.mView(0, 0), View.mView(1, 0), View.mView(2, 0)};
    const FVector Up{View.mView(0, 1), View.mView(1, 1), View.mView(2, 1)};
    for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
        for (const auto& Component : Actor->GetComponents()) {
            const auto* Primitive{Cast<UPrimitiveComponent>(Component.get())};
            const auto* Sprite{Cast<UBillboardComponent>(Component.get())};
            const auto* Text{Cast<UBillboardTextComponent>(Component.get())};
            if (!mShowFlags.mPrimitives || Actor->IsPendingDestroy() || Primitive == nullptr || !Primitive->ShouldCreateRenderState()) {
                continue;
            }
            if (Sprite != nullptr) {
                const FPrimitiveSceneProxy Proxy{Sprite->CreateSceneProxy()};
                if (mShowFlags.mSprites && RaycastMesh(&GetPrimitiveMesh(EPrimitiveType::Plane), MakeSpriteWorld(Proxy.mWorld, *Proxy.mSprite, View), Ray, Closest)) {
                    Result = {EHitProxyType::Object, Sprite->GetHandle()};
                }
                continue;
            }
            if (Text == nullptr) {
                if (RaycastMesh(&Primitive->GetMeshData(), Primitive->GetWorldMatrix(), Ray, Closest)) {
                    Result = {EHitProxyType::Object, Primitive->GetHandle()};
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
            if (Clip.w <= 0.0f || Clip.z < 0.0f || Clip.z > Clip.w || !Math::IntersectPlane(Ray, Transform.mLocation, View.mForward, Intersection)) {
                continue;
            }
            const float Distance{(Intersection - Ray.mOrigin).Dot(Ray.mDirection)};
            const FVector Offset{Intersection - Transform.mLocation};
            const float Horizontal{Offset.Dot(Right) / (Text->GetTextSize() * std::abs(Transform.mScale.y))};
            const float Vertical{Offset.Dot(Up) / (Text->GetTextSize() * std::abs(Transform.mScale.z))};
            if (Distance < Closest && Horizontal >= Minimum.x && Horizontal <= Maximum.x && Vertical >= Minimum.y && Vertical <= Maximum.y) {
                Closest = Distance;
                Result = {EHitProxyType::Object, Text->GetHandle()};
            }
        }
    }
    return Result;
}

FBox FEditorViewportClient::CalcComponentBounds(const UPrimitiveComponent* Component, const FSceneView& View) const {
    if (const auto* Sprite{Cast<UBillboardComponent>(Component)}) {
        const FPrimitiveSceneProxy Proxy{Sprite->CreateSceneProxy()};
        return FBox{{-0.5f, -0.5f, -0.001f}, {0.5f, 0.5f, 0.001f}}.Transformed(MakeSpriteWorld(Proxy.mWorld, *Proxy.mSprite, View));
    }
    const auto* Text{Cast<UBillboardTextComponent>(Component)};
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

bool FEditorViewportClient::DropMesh(const std::filesystem::path& Path, const FVector2& Position, const FVector2& Size) {
    UWorld* World{mContext->GetWorld()};
    FProject* Project{mContext->GetProject()};
    if (!mContext->CanEditWorld() || mGizmo.IsDragging() || World == nullptr || Project == nullptr) {
        return false;
    }
    const FSceneView View{CalcSceneView(Size)};
    FRay Ray{};
    if (!View.Deproject(Position, Ray)) {
        return false;
    }
    FWorldHit Hit{};
    FVector Location{mViewType == EEditorViewportType::Perspective ? Ray.mOrigin + Ray.mDirection * 6.0f : mOrbitPivot};
    const bool OnSurface{World->Raycast(Ray, Hit)};
    const bool OnGround{!OnSurface && Math::IntersectPlane(Ray, mViewType == EEditorViewportType::Perspective ? FVector{} : mOrbitPivot, mViewType == EEditorViewportType::Perspective ? FVector{0.0f, 0.0f, 1.0f} : View.mForward, Location)};
    if (OnSurface) {
        Location = Hit.mPosition;
    }
    FString Error{};
    const UStaticMesh* Mesh{Project->GetAssets()->LoadMesh(Path, Error)};
    if (Mesh == nullptr) {
        mContext->AddLog(Error);
        return false;
    }
    if (OnSurface || (OnGround && mViewType == EEditorViewportType::Perspective)) {
        Location.z -= Mesh->GetBounds().mMinimum.z;
    }
    return mContext->SpawnMesh(Path, FTransform{Location, {}, {1.0f, 1.0f, 1.0f}}) != nullptr;
}

EViewMode FEditorViewportClient::GetViewMode() const {
    return mViewMode;
}

void FEditorViewportClient::SetViewMode(EViewMode Mode) {
    if (Mode == EViewMode::Lit || Mode == EViewMode::Wireframe) {
        mViewMode = Mode;
    }
}

const FEngineShowFlags& FEditorViewportClient::GetShowFlags() const {
    return mShowFlags;
}

void FEditorViewportClient::SetShowFlags(const FEngineShowFlags& Flags) {
    mGizmo.Cancel();
    mShowFlags = Flags;
}

float FEditorViewportClient::GetLineThickness() const {
    return mLineThickness;
}

bool FEditorViewportClient::SetLineThickness(float Thickness) {
    if (!std::isfinite(Thickness) || Thickness < 0.5f || Thickness > 10.0f) {
        return false;
    }
    mLineThickness = Thickness;
    return true;
}

float FEditorViewportClient::GetGridSpacing() const {
    return mGridSpacing;
}

bool FEditorViewportClient::SetGridSpacing(float Spacing) {
    if (!std::isfinite(Spacing) || Spacing < 0.01f || Spacing > 1000.0f) {
        return false;
    }
    mGridSpacing = Spacing;
    return true;
}

float FEditorViewportClient::GetCameraSensitivity() const {
    return mCameraSensitivity;
}

bool FEditorViewportClient::SetCameraSensitivity(float Sensitivity) {
    if (!std::isfinite(Sensitivity) || Sensitivity < 0.01f || Sensitivity > 2.0f) {
        return false;
    }
    mCameraSensitivity = Sensitivity;
    return true;
}

float FEditorViewportClient::GetCameraSpeed() const {
    return mCameraSpeed;
}

bool FEditorViewportClient::SetCameraSpeed(float Speed) {
    if (!std::isfinite(Speed) || Speed < 0.1f || Speed > 128.0f) {
        return false;
    }
    mCameraSpeed = Speed;
    return true;
}
