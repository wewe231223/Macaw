#include "Core/Math.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FRay.h"

#include <algorithm>
#include <cmath>
#include <numbers>

float Math::ToRadians(float Degrees) {
    return Degrees * std::numbers::pi_v<float> / 180.0f;
}

float Math::ToDegrees(float Radians) {
    return Radians * 180.0f / std::numbers::pi_v<float>;
}

bool Math::NearlyEqual(float Left, float Right, float Tolerance) {
    return std::abs(Left - Right) <= Tolerance;
}

bool Math::IntersectPlane(const FRay& Ray, const FVector& Point, const FVector& Normal, FVector& Hit) {
    const float Denominator{Ray.mDirection.Dot(Normal)};

    if (std::abs(Denominator) < 0.000001f) {
        return false;
    }

    const float Distance{(Point - Ray.mOrigin).Dot(Normal) / Denominator};

    if (Distance < 0.0f) {
        return false;
    }

    Hit = Ray.mOrigin + Ray.mDirection * Distance;

    return true;
}

bool Math::IntersectTriangle(const FRay& Ray, const FVector& A, const FVector& B, const FVector& C, float& Distance) {
    const FVector EdgeA{B - A};
    const FVector EdgeB{C - A};
    const FVector P{Ray.mDirection.Cross(EdgeB)};
    const float Determinant{EdgeA.Dot(P)};

    if (std::abs(Determinant) < 0.0000001f) {
        return false;
    }

    const float Inverse{1.0f / Determinant};
    const FVector T{Ray.mOrigin - A};
    const float U{T.Dot(P) * Inverse};

    if (U < 0.0f || U > 1.0f) {
        return false;
    }

    const FVector Q{T.Cross(EdgeA)};
    const float V{Ray.mDirection.Dot(Q) * Inverse};

    if (V < 0.0f || U + V > 1.0f) {
        return false;
    }

    Distance = EdgeB.Dot(Q) * Inverse;

    return Distance >= 0.0f;
}

bool Math::Project(const FVector& Position, const FMatrix& ViewProjection, const FVector2& Size, FVector2& Screen) {
    const float W{Position.mX * ViewProjection.mValues[0][3] + Position.mY * ViewProjection.mValues[1][3] + Position.mZ * ViewProjection.mValues[2][3] + ViewProjection.mValues[3][3]};

    if (W <= 0.00001f || Size.mX <= 0.0f || Size.mY <= 0.0f) {
        return false;
    }

    const FVector Clip{ViewProjection.TransformPosition(Position)};
    Screen = {(Clip.mX + 1.0f) * Size.mX * 0.5f, (1.0f - Clip.mY) * Size.mY * 0.5f};

    return std::isfinite(Screen.mX) && std::isfinite(Screen.mY);
}

bool Math::Deproject(const FVector2& Screen, const FVector2& Size, const FMatrix& ViewProjection, FRay& Ray) {
    FMatrix Inverse{};

    if (Size.mX <= 0.0f || Size.mY <= 0.0f || !ViewProjection.TryInverse(Inverse)) {
        return false;
    }

    const float X{Screen.mX * 2.0f / Size.mX - 1.0f};
    const float Y{1.0f - Screen.mY * 2.0f / Size.mY};
    Ray.mOrigin = Inverse.TransformPosition({X, Y, 0.0f});
    Ray.mDirection = (Inverse.TransformPosition({X, Y, 1.0f}) - Ray.mOrigin).Normalized();

    return Ray.mOrigin.IsFinite() && Ray.mDirection.Length() > 0.0f;
}

float Math::DistanceToSegment(const FVector2& Point, const FVector2& Start, const FVector2& End) {
    const float X{End.mX - Start.mX};
    const float Y{End.mY - Start.mY};
    const float LengthSquared{X * X + Y * Y};

    const float T{LengthSquared > 0.00001f ? std::clamp(((Point.mX - Start.mX) * X + (Point.mY - Start.mY) * Y) / LengthSquared, 0.0f, 1.0f) : 0.0f};

    return std::hypot(Point.mX - Start.mX - X * T, Point.mY - Start.mY - Y * T);
}
