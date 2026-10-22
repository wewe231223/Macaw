#pragma once

#include "Core/Math/FVector2.h"
#include "Core/Math/FRay.h"
#include "Core/Math/FMatrix.h"

namespace Math {
    float ToRadians(float Degrees);
    float ToDegrees(float Radians);
    bool NearlyEqual(float Left, float Right, float Tolerance = 0.0001f);
    bool IntersectPlane(const FRay& Ray, const FVector& Point, const FVector& Normal, FVector& Hit);
    bool IntersectTriangle(const FRay& Ray, const FVector& A, const FVector& B, const FVector& C, float& Distance);
    bool Project(const FVector& Position, const FMatrix& ViewProjection, const FVector2& Size, FVector2& Screen);
    bool Deproject(const FVector2& Screen, const FVector2& Size, const FMatrix& ViewProjection, FRay& Ray);
    float DistanceToSegment(const FVector2& Point, const FVector2& Start, const FVector2& End);
}
