/**
 * @file FSceneView.cpp
 * @brief 카메라 행렬과 화면 좌표의 월드 광선 변환을 제공합니다.
 * @ingroup Engine
 */

#include "Engine/FSceneView.h"
#include "Core/Math.h"
#include "Core/Math/FRay.h"

#include <cmath>

bool FSceneView::Deproject(const FVector2& Position, FRay& Ray) const {
    if (!mOrthographic) {
        return Math::Deproject(Position, mSize, mViewProjection, Ray);
    }

    if (!std::isfinite(Position.x) || !std::isfinite(Position.y) || !std::isfinite(mSize.x) || !std::isfinite(mSize.y) || mSize.x <= 0.0f || mSize.y <= 0.0f || mProjection(0, 0) <= 0.0f || mProjection(1, 1) <= 0.0f || mProjection(2, 2) <= 0.0f) {
        return false;
    }

    const float X{(Position.x * 2.0f / mSize.x - 1.0f) / mProjection(0, 0)};
    const float Y{(1.0f - Position.y * 2.0f / mSize.y) / mProjection(1, 1)};
    const float NearPlane{-mProjection(3, 2) / mProjection(2, 2)};
    const FVector Right{mView(0, 0), mView(1, 0), mView(2, 0)};
    const FVector Up{mView(0, 1), mView(1, 1), mView(2, 1)};
    Ray.mOrigin = mLocation + Right * X + Up * Y + mForward * NearPlane;
    Ray.mDirection = mForward;

    return Ray.mDirection.Normalize() && Ray.mOrigin.IsFinite();
}
