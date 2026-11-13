/**
 * @file FViewerCamera.cpp
 * @brief 에셋 경계를 기준으로 Fit, 회전, 이동과 확대를 수행하는 궤도 카메라입니다.
 * @ingroup Viewer
 */

#include "Viewer/FViewerCamera.h"

#include "Core/Math.h"
#include "Core/Math/FQuat.h"

#include <algorithm>
#include <cmath>

using namespace Viewer;

FViewerCamera::FViewerCamera() = default;

bool FViewerCamera::Fit(const Core::FBox& Bounds, const Core::FVector2& Size, float Padding) {
    if (!std::isfinite(Size.x) || !std::isfinite(Size.y) || Size.x <= 0.0f || Size.y <= 0.0f || !std::isfinite(Padding) || Padding < 1.0f) {
        return false;
    }

    const Core::FVector Center{(Bounds.mMinimum + Bounds.mMaximum) * 0.5f};
    const Core::FVector Extent{(Bounds.mMaximum - Bounds.mMinimum) * 0.5f};

    if (!std::isfinite(Center.x) || !std::isfinite(Center.y) || !std::isfinite(Center.z) || !std::isfinite(Extent.x) || !std::isfinite(Extent.y) || !std::isfinite(Extent.z) || Extent.x < 0.0f || Extent.y < 0.0f || Extent.z < 0.0f) {
        return false;
    }

    const Core::FQuat Rotation{mRotation.Quaternion()};
    const Core::FVector Forward{Rotation.ForwardVector()};
    const Core::FVector Right{Rotation.RightVector()};
    const Core::FVector Up{Rotation.UpVector()};
    const float TanVertical{std::tan(Core::Math::ToRadians(mFieldOfView) * 0.5f)};
    const float TanHorizontal{TanVertical * Size.x / Size.y};
    const float Radius{std::max(Extent.Length(), 0.01f)};
    const float MinimumDepth{std::max(Radius * 0.01f, 0.0001f)};
    float Distance{MinimumDepth};

    for (Core::Uint32 Corner{}; Corner < 8; ++Corner) {
        const Core::FVector Offset{(Corner & 1) != 0 ? Extent.x : -Extent.x, (Corner & 2) != 0 ? Extent.y : -Extent.y, (Corner & 4) != 0 ? Extent.z : -Extent.z};
        const float Depth{Offset.Dot(Forward)};

        Distance = std::max({Distance, Padding * std::abs(Offset.Dot(Right)) / TanHorizontal - Depth, Padding * std::abs(Offset.Dot(Up)) / TanVertical - Depth, MinimumDepth - Depth});
    }

    if (!std::isfinite(Distance)) {
        return false;
    }

    mCenter = Center;
    mRadius = Radius;
    mDistance = std::max(Distance, Radius * 0.02f);

    return true;
}

Engine::FSceneView FViewerCamera::CalcSceneView(const Core::FVector2& Size) const {
    const Core::FQuat Rotation{mRotation.Quaternion()};
    Engine::FSceneView View{};
    View.mSize = Size;
    View.mLocation = mCenter - Rotation.ForwardVector() * mDistance;
    View.mForward = Rotation.ForwardVector();
    View.mFieldOfView = mFieldOfView;
    View.mView = Core::FMatrix::LookTo(View.mLocation, View.mForward, Rotation.UpVector());
    View.mProjection = Core::FMatrix::Perspective(mFieldOfView, std::max(Size.x, 1.0f) / std::max(Size.y, 1.0f), std::max(mRadius * 0.001f, 0.00001f), std::max(mDistance + mRadius * 3.0f, mRadius * 10.0f));
    View.mViewProjection = View.mView * View.mProjection;

    return View;
}

void FViewerCamera::Orbit(const Core::FVector2& Delta) {
    if (std::isfinite(Delta.x) && std::isfinite(Delta.y)) {
        mRotation.yaw += Delta.x * 0.3f;
        mRotation.pitch = std::clamp(mRotation.pitch - Delta.y * 0.3f, -89.0f, 89.0f);
    }
}

void FViewerCamera::Pan(const Core::FVector2& Delta, float Height) {
    if (std::isfinite(Delta.x) && std::isfinite(Delta.y) && std::isfinite(Height) && Height > 0.0f) {
        const Core::FQuat Rotation{mRotation.Quaternion()};

        mCenter += (Rotation.RightVector() * -Delta.x + Rotation.UpVector() * Delta.y) * (mDistance / Height);
    }
}

void FViewerCamera::Zoom(float Delta) {
    if (std::isfinite(Delta)) {
        mDistance = std::clamp(mDistance * std::exp(-Delta * 0.15f), mRadius * 0.02f, mRadius * 1000.0f);
    }
}

void FViewerCamera::SetRotation(const Core::FRotator& Rotation) {
    if (std::isfinite(Rotation.pitch) && std::isfinite(Rotation.yaw) && std::isfinite(Rotation.roll)) {
        mRotation = Rotation;
    }
}

void FViewerCamera::SetFieldOfView(float FieldOfView) {
    if (std::isfinite(FieldOfView)) {
        mFieldOfView = std::clamp(FieldOfView, 1.0f, 170.0f);
    }
}

const Core::FRotator& FViewerCamera::GetRotation() const {
    return mRotation;
}

const Core::FVector& FViewerCamera::GetCenter() const {
    return mCenter;
}

float FViewerCamera::GetRadius() const {
    return mRadius;
}

float FViewerCamera::GetDistance() const {
    return mDistance;
}
