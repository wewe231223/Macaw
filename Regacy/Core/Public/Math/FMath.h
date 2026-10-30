/**
 * @file FMath.h
 * @brief 엔진 수학 타입, 기하 타입 별칭과 스칼라 비교·각도 변환 함수를 제공합니다.
 */

#pragma once

#include <cmath>
#include "Math/FVector.h"
#include "Math/FVector2.h"
#include "Math/FVector4.h"
#include "Math/FMatrix.h"
#include "Math/FRotator.h"
#include "Math/FQuat.h"
#include "Math/FVectorRegister.h"
#include "Math/FMatrixRegister.h"

/**
 * @brief 라디안·도 변환에 사용하는 단정밀도 원주율 근사값입니다.
 */
#define PI 3.141592f

/**
 * @brief DirectX SimpleMath의 평면 타입 별칭입니다.
 */
using FPlane = DirectX::SimpleMath::Plane;

/**
 * @brief DirectX SimpleMath의 시작점과 방향으로 표현하는 광선 타입 별칭입니다.
 */
using FRay = DirectX::SimpleMath::Ray;

/**
 * @brief DirectX의 시야 절두체 타입 별칭입니다.
 */
using FFrustum = DirectX::BoundingFrustum;

/**
 * @brief 스칼라의 허용 오차 비교, 범위 제한과 각도 단위 변환 함수를 제공합니다.
 */
namespace FMath {
    //static float PI = std::acosf(-1);

    /**
     * @brief 값의 절댓값이 허용 오차 이하인지 검사합니다.
     * @param Value 검사할 값입니다.
     * @param ErrorTolerance 음수가 아닌 절대 허용 오차입니다.
     * @return 절댓값이 허용 오차 이하이면 true입니다.
     */
    static inline bool IsNearlyZero(float Value, float ErrorTolerance = 1e-8f) {
        return std::abs(Value) <= ErrorTolerance;
    }

    /**
     * @brief 두 값의 차이가 절대 허용 오차 이내인지 검사합니다.
     * @param Value1 첫 번째 값입니다.
     * @param Value2 두 번째 값입니다.
     * @param ErrorTolerance 음수가 아닌 절대 허용 오차입니다.
     * @return 차이의 절댓값이 허용 오차 이하이면 true입니다.
     */
    static inline bool IsNearlyEqual(float Value1, float Value2, float ErrorTolerance = 1e-4f) {
        return std::abs(Value1 - Value2) <= ErrorTolerance;
    }

    /**
     * @brief 값을 양 끝점을 포함하는 범위로 제한합니다.
     * @param Value 제한할 값입니다.
     * @param Min 범위의 최솟값입니다.
     * @param Max 범위의 최댓값입니다.
     * @pre Min은 Max 이하이어야 합니다.
     * @return 범위에 맞게 제한한 값입니다.
     */
    static inline float Clamp(float Value, float Min, float Max) {
        if (Value < Min)
            return Min;

        if (Value > Max)
            return Max;

        return Value;
    }

    /**
     * @brief 라디안을 도 단위 각도로 변환합니다.
     * @param Radian 라디안 단위 각도입니다.
     * @return 도 단위 각도입니다.
     */
    static inline float RadiansToDegrees(float Radian) {
        return Radian * (180.0f / PI);
    }

    /**
     * @brief 도 단위 각도를 라디안으로 변환합니다.
     * @param Degree 도 단위 각도입니다.
     * @return 라디안 단위 각도입니다.
     */
    static inline float DegreesToRadians(float Degree) {
        return Degree * (PI / 180.0f);
    }
}
