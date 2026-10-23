/**
 * @file ViewportClient.cpp
 * @brief 에디터 카메라, 선택과 기즈모 입력을 처리합니다.
 * @ingroup Editor
 */

#include "Editor/ViewportClient.h"
#include "RenderCore/IRendererModule.h"
#include "Core/Math.h"
#include "Engine/Component/UPrimitiveComponent.h"
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
#include "CoreUObject/Reflection/Cast.h"
#include "Editor/EditorEngine.h"

#include <algorithm>
#include <cmath>

/**
 * @brief FEditorViewportClient의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FEditorViewportClient::FEditorViewportClient(UEditorEngine& Engine)
	: mEngine{&Engine},
	  mGizmo{Engine} {
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FEditorViewportClient::~FEditorViewportClient() = default;

/**
 * @brief 현재 프레임의 뷰포트 입력을 처리합니다.
 */
void FEditorViewportClient::Input(const FViewportInput& Input, const FVector2& Size, float DeltaTime) {
    if (!mGizmo.IsDragging() && NavigateCamera(Input, Size, DeltaTime)) {
        return;
    }

    if (Input.mCycleGizmo && Input.mFocused && !Input.mAlt && !Input.mLeftDown && !mGizmo.IsDragging()) {
        mGizmo.SetMode(static_cast<EGizmoMode>((static_cast<Int32>(mGizmo.GetMode()) + 1) % 3));
    }

    const FSceneView View{CalcSceneView(Size)};
    const bool GizmoConsumed{mGizmo.Input(Input, View)};

    if (Input.mDelete && Input.mFocused && !mGizmo.IsDragging()) {
        mEngine->DestroySelectedActor();
        return;
    }

    if (Input.mLeftPressed && Input.mHovered && !Input.mAlt && !GizmoConsumed && mEngine->GetWorld() != nullptr) {
        FRay Ray{};
        FWorldHit Hit{};

        if (Math::Deproject(Input.mMouse, Size, View.mViewProjection, Ray) && mEngine->GetWorld()->Raycast(Ray, Hit)) {
            mEngine->SelectActor(Hit.mActor);
        } else {
            mEngine->ClearSelection();
        }
    }
}

/**
 * @brief 뷰포트 입력으로 카메라 이동, 회전, 팬 또는 오빗을 적용합니다.
 */
bool FEditorViewportClient::NavigateCamera(const FViewportInput& Input, const FVector2& Size, float DeltaTime) {
    if (!Input.mHovered && !Input.mFocused) {
        return false;
    }

    if (Input.mFocusSelection && Input.mFocused && !Input.mRightDown && !Input.mAlt) {
        FocusSelection(Size);
    }

    const FQuat Rotation{mCameraRotation.Quaternion()};

    if (Input.mWheel != 0.0f && Input.mHovered) {
        if (Input.mRightDown && !Input.mAlt) {
            mCameraSpeed = std::clamp(mCameraSpeed * std::pow(1.25f, Input.mWheel), 0.1f, 128.0f);
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
            mCameraRotation.yaw += Input.mDelta.x * 0.2f;
            mCameraRotation.pitch = std::clamp(mCameraRotation.pitch - Input.mDelta.y * 0.2f, -89.0f, 89.0f);
        } else {
            Distance = std::clamp(Distance * std::exp((Input.mDelta.y - Input.mDelta.x) * 0.01f), 0.1f, 100000.0f);
        }

        mCameraLocation = mOrbitPivot - mCameraRotation.Vector() * Distance;

        return true;
    }

    if (Input.mRightDown) {
        mCameraRotation.yaw += Input.mDelta.x * 0.2f;
        mCameraRotation.pitch = std::clamp(mCameraRotation.pitch - Input.mDelta.y * 0.2f, -89.0f, 89.0f);

        const FQuat FlightRotation{mCameraRotation.Quaternion()};
        const FVector LocalMovement{static_cast<float>(Input.mForward) - static_cast<float>(Input.mBackward), static_cast<float>(Input.mRight) - static_cast<float>(Input.mLeft), 0.0f};
        FVector Movement{FlightRotation * LocalMovement};
        Movement.z += static_cast<float>(Input.mUp) - static_cast<float>(Input.mDown);

        mCameraLocation += Movement.Normalized() * std::max(DeltaTime, 0.0f) * mCameraSpeed * (Input.mFast ? 3.0f : 1.0f);

        return true;
    }

    return Input.mAlt || Input.mMiddleDown || (Input.mWheel != 0.0f && Input.mHovered);
}

/**
 * @brief 선택한 액터가 화면에 들어오도록 카메라 위치와 중심을 조정합니다.
 */
void FEditorViewportClient::FocusSelection(const FVector2& Size) {
    const AActor* Actor{mEngine->GetSelectedActor()};
    const auto* Component{Actor != nullptr ? Cast<UPrimitiveComponent>(Actor->GetRootComponent()) : nullptr};

    if (Component == nullptr) {
        return;
    }

    const FBox Bounds{Component->GetWorldBounds()};
    mOrbitPivot = Bounds.mMinimum.Lerp(Bounds.mMaximum, 0.5f);

    const float Radius{Bounds.mMinimum.Distance(Bounds.mMaximum) * 0.5f};
    const float HalfFieldOfView{std::atan(std::tan(Math::ToRadians(30.0f)) * std::min(std::max(Size.x, 1.0f) / std::max(Size.y, 1.0f), 1.0f))};
    const float Distance{std::max(0.5f, Radius / std::sin(HalfFieldOfView) * 1.2f)};

    mCameraLocation = mOrbitPivot - mCameraRotation.Vector() * Distance;
}

/**
 * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
 */
void FEditorViewportClient::Draw(FSceneViewport& Viewport) {
    UWorld* World{mEngine->GetWorld()};

    if (World == nullptr || World->GetScene() == nullptr) {
        return;
    }

    const FSceneView View{CalcSceneView(Viewport.GetSize())};

    mDrawInterface.Clear();

    mDrawInterface.DrawLine({}, {2.0f, 0.0f, 0.0f}, {1.0f, 0.25f, 0.2f}, true);
    mDrawInterface.DrawLine({}, {0.0f, 2.0f, 0.0f}, {0.3f, 0.9f, 0.4f}, true);
    mDrawInterface.DrawLine({}, {0.0f, 0.0f, 2.0f}, {0.3f, 0.55f, 1.0f}, true);

    if (const AActor* Actor{mEngine->GetSelectedActor()}; Actor != nullptr && Actor->GetRootComponent() != nullptr) {
        const FTransform& Transform{Actor->GetRootComponent()->GetTransform()};
        const std::array<FVector, 3> Axes{Transform.mRotation.ForwardVector(), Transform.mRotation.RightVector(), Transform.mRotation.UpVector()};
        const std::array<FVector, 3> Colors{{{0.65f, 0.15f, 0.12f}, {0.12f, 0.65f, 0.2f}, {0.15f, 0.35f, 0.8f}}};

        for (Int32 Index{}; Index < 3; ++Index) {
            mDrawInterface.DrawLine(Transform.mLocation, Transform.mLocation + Axes[Index] * 0.6f, Colors[Index]);
        }
    }

    mGizmo.Draw(View, mDrawInterface);

    const FSceneViewFamily Family{World->GetScene(), Viewport.GetRenderTarget(), View, &mDrawInterface};
    mEngine->GetRenderer()->BeginRenderingViewFamily(Family);
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

    View.mView = FMatrix::LookTo(mCameraLocation, View.mForward, Rotation.UpVector());

    View.mProjection = FMatrix::Perspective(View.mFieldOfView, std::max(Size.x, 1.0f) / std::max(Size.y, 1.0f), 0.05f, 10000.0f);
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
}

/**
 * @brief 유효한 위치와 회전으로 에디터 카메라를 설정합니다.
 */
void FEditorViewportClient::SetCamera(const FVector& Location, const FRotator& Rotation) {
    mCameraLocation = Location;
    mCameraRotation = Rotation;
    mOrbitPivot = Location + Rotation.Vector() * 10.0f;
}

/**
 * @brief 에디터 카메라의 월드 위치를 반환합니다.
 */
const FVector& FEditorViewportClient::GetCameraLocation() const {
    return mCameraLocation;
}

/**
 * @brief 뷰포트 변환 편집 기즈모를 반환합니다.
 */
FTransformGizmo& FEditorViewportClient::GetGizmo() {
    return mGizmo;
}
