/**
 * @file MathUtility.cpp
 * @brief 각도 변환, 교차 검사와 화면 투영 유틸리티를 제공합니다.
 * @ingroup Core
 */

#include "Core/Math/MathUtility.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector4.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FRay.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Core {

    /**
     * @fn float Core::Math::ToRadians(float Degrees)
     * @brief 도 단위 각도를 라디안으로 변환합니다.
     */
    float Core::Math::ToRadians(float Degrees) {
        return Degrees * std::numbers::pi_v<float> / 180.0f;
    }

    /**
     * @fn float Core::Math::ToDegrees(float Radians)
     * @brief 라디안 단위 각도를 도로 변환합니다.
     */
    float Core::Math::ToDegrees(float Radians) {
        return Radians * 180.0f / std::numbers::pi_v<float>;
    }

    /**
     * @fn bool Core::Math::NearlyEqual(float Left, float Right, float Tolerance)
     * @brief 두 스칼라의 차이가 지정한 허용 오차 이내인지 검사합니다.
     */
    bool Core::Math::NearlyEqual(float Left, float Right, float Tolerance) {
        return std::abs(Left - Right) <= Tolerance;
    }

    /**
     * @fn bool Core::Math::IntersectPlane(const Core::FRay& Ray, const Core::FVector& Point, const Core::FVector& Normal, Core::FVector& Hit)
     * @brief 광선과 평면의 앞쪽 교점을 계산합니다.
     */
    bool Core::Math::IntersectPlane(const Core::FRay& Ray, const Core::FVector& Point, const Core::FVector& Normal, Core::FVector& Hit) {
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

    /**
     * @fn bool Core::Math::IntersectTriangle(const Core::FRay& Ray, const Core::FVector& A, const Core::FVector& B, const Core::FVector& C, float& Distance)
     * @brief 광선과 삼각형의 교차를 검사하여 광선상의 거리를 구합니다.
     */
    bool Core::Math::IntersectTriangle(const Core::FRay& Ray, const Core::FVector& A, const Core::FVector& B, const Core::FVector& C, float& Distance) {
        const Core::FVector EdgeA{B - A};
        const Core::FVector EdgeB{C - A};
        const Core::FVector P{Ray.mDirection.Cross(EdgeB)};
        const float Determinant{EdgeA.Dot(P)};

        if (std::abs(Determinant) < 0.0000001f) {
            return false;
        }

        const float Inverse{1.0f / Determinant};
        const Core::FVector T{Ray.mOrigin - A};
        const float U{T.Dot(P) * Inverse};

        if (U < 0.0f || U > 1.0f) {
            return false;
        }

        const Core::FVector Q{T.Cross(EdgeA)};
        const float V{Ray.mDirection.Dot(Q) * Inverse};

        if (V < 0.0f || U + V > 1.0f) {
            return false;
        }

        Distance = EdgeB.Dot(Q) * Inverse;

        return Distance >= 0.0f;
    }

    /**
     * @fn bool Core::Math::Project(const Core::FVector& Position, const Core::FMatrix& ViewProjection, const Core::FVector2& Size, Core::FVector2& Screen)
     * @brief 월드 위치를 뷰포트 픽셀 좌표로 투영합니다.
     */
    bool Core::Math::Project(const Core::FVector& Position, const Core::FMatrix& ViewProjection, const Core::FVector2& Size, Core::FVector2& Screen) {
        const Core::FVector4 Clip{Core::FVector4{Position, 1.0f} * ViewProjection};
        Core::FVector PositionNdc{};

        if (Clip.w <= 0.00001f || Size.x <= 0.0f || Size.y <= 0.0f || !Clip.TryPerspectiveDivide(PositionNdc)) {
            return false;
        }

        Screen = {(PositionNdc.x + 1.0f) * Size.x * 0.5f, (1.0f - PositionNdc.y) * Size.y * 0.5f};

        return Screen.IsFinite();
    }

    /**
     * @fn bool Core::Math::Deproject(const Core::FVector2& Screen, const Core::FVector2& Size, const Core::FMatrix& ViewProjection, Core::FRay& Ray)
     * @brief 뷰포트 픽셀 좌표에서 월드 공간 피킹 광선을 생성합니다.
     */
    bool Core::Math::Deproject(const Core::FVector2& Screen, const Core::FVector2& Size, const Core::FMatrix& ViewProjection, Core::FRay& Ray) {
        Core::FMatrix Inverse{};

        if (Size.x <= 0.0f || Size.y <= 0.0f || !ViewProjection.TryInverse(Inverse)) {
            return false;
        }

        const float X{Screen.x * 2.0f / Size.x - 1.0f};
        const float Y{1.0f - Screen.y * 2.0f / Size.y};
        Ray.mOrigin = Inverse.TransformPosition({X, Y, 0.0f});
        Ray.mDirection = Inverse.TransformPosition({X, Y, 1.0f}) - Ray.mOrigin;

        return Ray.mDirection.Normalize() && Ray.mOrigin.IsFinite();
    }

    /**
     * @fn float Core::Math::DistanceToSegment(const Core::FVector2& Point, const Core::FVector2& Start, const Core::FVector2& End)
     * @brief 2차원 점에서 유한 선분까지의 최단 거리를 구합니다.
     */
    float Core::Math::DistanceToSegment(const Core::FVector2& Point, const Core::FVector2& Start, const Core::FVector2& End) {
        const Core::FVector2 Segment{End - Start};
        const float LengthSquared{Segment.LengthSquared()};
        const float T{LengthSquared > 0.00001f ? std::clamp((Point - Start).Dot(Segment) / LengthSquared, 0.0f, 1.0f) : 0.0f};

        return Point.Distance(Start.Lerp(End, T));
    }

}
