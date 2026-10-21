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

FEditorViewportClient::FEditorViewportClient(UEditorEngine& Engine)
	: mEngine{&Engine},
	  mGizmo{Engine} {
}

FEditorViewportClient::~FEditorViewportClient() = default;

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

bool FEditorViewportClient::NavigateCamera(const FViewportInput& Input, const FVector2& Size, float DeltaTime) {
    if (!Input.mHovered && !Input.mFocused) {
        return false;
    }

    if (Input.mFocusSelection && Input.mFocused && !Input.mRightDown && !Input.mAlt) {
        FocusSelection(Size);
    }

    const FMatrix Rotation{FMatrix::Rotation(mCameraRotation)};

    if (Input.mWheel != 0.0f && Input.mHovered) {
        if (Input.mRightDown && !Input.mAlt) {
            mCameraSpeed = std::clamp(mCameraSpeed * std::pow(1.25f, Input.mWheel), 0.1f, 128.0f);
        } else {
            mCameraLocation += Rotation.GetAxis(0) * Input.mWheel * mCameraSpeed * 0.5f;
        }
    }

    if (Input.mMiddleDown || (Input.mLeftDown && Input.mRightDown && !Input.mAlt)) {
        const float Distance{std::max((mCameraLocation - mOrbitPivot).Length(), 1.0f)};
        const float PixelScale{2.0f * Distance * std::tan(Math::ToRadians(30.0f)) / std::max(Size.mY, 1.0f)};
        const FVector Pan{(Rotation.GetAxis(1) * -Input.mDelta.mX + Rotation.GetAxis(2) * Input.mDelta.mY) * PixelScale};
        mCameraLocation += Pan;
        mOrbitPivot += Pan;

        return true;
    }

    if (Input.mAlt && (Input.mLeftDown || Input.mRightDown)) {
        float Distance{std::max((mCameraLocation - mOrbitPivot).Length(), 0.1f)};
        const FVector Forward{(mOrbitPivot - mCameraLocation).Normalized()};

        if (Forward.Length() > 0.01f) {
            mCameraRotation.mYaw = Math::ToDegrees(std::atan2(Forward.mY, Forward.mX));
            mCameraRotation.mPitch = std::clamp(Math::ToDegrees(std::atan2(Forward.mZ, std::hypot(Forward.mX, Forward.mY))), -89.0f, 89.0f);
        }

        if (Input.mLeftDown) {
            mCameraRotation.mYaw += Input.mDelta.mX * 0.2f;
            mCameraRotation.mPitch = std::clamp(mCameraRotation.mPitch - Input.mDelta.mY * 0.2f, -89.0f, 89.0f);
        } else {
            Distance = std::clamp(Distance * std::exp((Input.mDelta.mY - Input.mDelta.mX) * 0.01f), 0.1f, 100000.0f);
        }

        mCameraLocation = mOrbitPivot - FMatrix::Rotation(mCameraRotation).GetAxis(0) * Distance;

        return true;
    }

    if (Input.mRightDown) {
        mCameraRotation.mYaw += Input.mDelta.mX * 0.2f;
        mCameraRotation.mPitch = std::clamp(mCameraRotation.mPitch - Input.mDelta.mY * 0.2f, -89.0f, 89.0f);

        const FMatrix FlightRotation{FMatrix::Rotation(mCameraRotation)};
        FVector Movement{};

        if (Input.mForward) {
            Movement += FlightRotation.GetAxis(0);
        }

        if (Input.mBackward) {
            Movement += -FlightRotation.GetAxis(0);
        }

        if (Input.mRight) {
            Movement += FlightRotation.GetAxis(1);
        }

        if (Input.mLeft) {
            Movement += -FlightRotation.GetAxis(1);
        }

        if (Input.mUp) {
            Movement += FVector{0.0f, 0.0f, 1.0f};
        }

        if (Input.mDown) {
            Movement += FVector{0.0f, 0.0f, -1.0f};
        }

        mCameraLocation += Movement.Normalized() * std::max(DeltaTime, 0.0f) * mCameraSpeed * (Input.mFast ? 3.0f : 1.0f);

        return true;
    }

    return Input.mAlt || Input.mMiddleDown || (Input.mWheel != 0.0f && Input.mHovered);
}

void FEditorViewportClient::FocusSelection(const FVector2& Size) {
    const AActor* Actor{mEngine->GetSelectedActor()};
    const auto* Component{Actor != nullptr ? Cast<UPrimitiveComponent>(Actor->GetRootComponent()) : nullptr};

    if (Component == nullptr) {
        return;
    }

    const FBox Bounds{Component->GetWorldBounds()};
    mOrbitPivot = (Bounds.mMinimum + Bounds.mMaximum) * 0.5f;

    const float Radius{(Bounds.mMaximum - Bounds.mMinimum).Length() * 0.5f};
    const float HalfFieldOfView{std::atan(std::tan(Math::ToRadians(30.0f)) * std::min(std::max(Size.mX, 1.0f) / std::max(Size.mY, 1.0f), 1.0f))};
    const float Distance{std::max(0.5f, Radius / std::sin(HalfFieldOfView) * 1.2f)};

    mCameraLocation = mOrbitPivot - FMatrix::Rotation(mCameraRotation).GetAxis(0) * Distance;
}

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
        const FMatrix Rotation{FMatrix::Rotation(Transform.mRotation)};
        constexpr std::array<FVector, 3> Colors{{{0.65f, 0.15f, 0.12f}, {0.12f, 0.65f, 0.2f}, {0.15f, 0.35f, 0.8f}}};

        for (Int32 Index{}; Index < 3; ++Index) {
            mDrawInterface.DrawLine(Transform.mLocation, Transform.mLocation + Rotation.GetAxis(Index) * 0.6f, Colors[Index]);
        }
    }

    mGizmo.Draw(View, mDrawInterface);

    const FSceneViewFamily Family{World->GetScene(), Viewport.GetRenderTarget(), View, &mDrawInterface};
    mEngine->GetRenderer()->BeginRenderingViewFamily(Family);
}

FSceneView FEditorViewportClient::CalcSceneView(const FVector2& Size) const {
    const FMatrix Rotation{FMatrix::Rotation(mCameraRotation)};

    FSceneView View{};
    View.mLocation = mCameraLocation;
    View.mForward = Rotation.GetAxis(0).Normalized();
    View.mSize = Size;

    View.mView = FMatrix::LookTo(mCameraLocation, View.mForward, {0.0f, 0.0f, 1.0f});

    View.mProjection = FMatrix::Perspective(View.mFieldOfView, std::max(Size.mX, 1.0f) / std::max(Size.mY, 1.0f), 0.05f, 10000.0f);
    View.mViewProjection = View.mView * View.mProjection;

    return View;
}

void FEditorViewportClient::ResetCamera() {
    mCameraLocation = {-7.0f, -9.0f, 6.0f};
    mCameraRotation = {-26.0f, 50.0f, 0.0f};

    mOrbitPivot = {};
    mCameraSpeed = 4.0f;
}

void FEditorViewportClient::SetCamera(const FVector& Location, const FRotator& Rotation) {
    mCameraLocation = Location;
    mCameraRotation = Rotation;
    mOrbitPivot = Location + FMatrix::Rotation(Rotation).GetAxis(0) * 10.0f;
}

const FVector& FEditorViewportClient::GetCameraLocation() const {
    return mCameraLocation;
}

FTransformGizmo& FEditorViewportClient::GetGizmo() {
    return mGizmo;
}
