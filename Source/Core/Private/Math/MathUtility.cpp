#include "Core/Math/MathUtility.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector4.h"
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
    const FVector4 Clip{FVector4{Position, 1.0f} * ViewProjection};
    FVector PositionNdc{};

    if (Clip.w <= 0.00001f || Size.x <= 0.0f || Size.y <= 0.0f || !Clip.TryPerspectiveDivide(PositionNdc)) {
        return false;
    }

    Screen = {(PositionNdc.x + 1.0f) * Size.x * 0.5f, (1.0f - PositionNdc.y) * Size.y * 0.5f};

    return Screen.IsFinite();
}

bool Math::Deproject(const FVector2& Screen, const FVector2& Size, const FMatrix& ViewProjection, FRay& Ray) {
    FMatrix Inverse{};

    if (Size.x <= 0.0f || Size.y <= 0.0f || !ViewProjection.TryInverse(Inverse)) {
        return false;
    }

    const float X{Screen.x * 2.0f / Size.x - 1.0f};
    const float Y{1.0f - Screen.y * 2.0f / Size.y};
    Ray.mOrigin = Inverse.TransformPosition({X, Y, 0.0f});
    Ray.mDirection = Inverse.TransformPosition({X, Y, 1.0f}) - Ray.mOrigin;

    return Ray.mDirection.Normalize() && Ray.mOrigin.IsFinite();
}

float Math::DistanceToSegment(const FVector2& Point, const FVector2& Start, const FVector2& End) {
    const FVector2 Segment{End - Start};
    const float LengthSquared{Segment.LengthSquared()};
    const float T{LengthSquared > 0.00001f ? std::clamp((Point - Start).Dot(Segment) / LengthSquared, 0.0f, 1.0f) : 0.0f};

    return Point.Distance(Start.Lerp(End, T));
}
