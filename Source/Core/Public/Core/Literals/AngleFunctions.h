/**
 * @file AngleFunctions.h
 * @brief FAngle을 받는 삼각함수, 각도 정규화, 보간과 FRotator 변환 유틸리티를 정의합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/Literals/UnitLiterals.h"
#include "Core/Math/FRotator.h"

#include <cmath>
#include <numbers>

namespace Core::Math {

    /**
     * @brief 각도의 사인 값을 반환합니다.
     * @param[in] Angle 각도입니다.
     * @return 사인 값입니다.
     */
    template <typename T>
    T Sin(Core::TQuantity<Core::Dim::Angle, T> Angle) {
        return std::sin(Angle.GetBase());
    }

    /**
     * @brief 각도의 코사인 값을 반환합니다.
     * @param[in] Angle 각도입니다.
     * @return 코사인 값입니다.
     */
    template <typename T>
    T Cos(Core::TQuantity<Core::Dim::Angle, T> Angle) {
        return std::cos(Angle.GetBase());
    }

    /**
     * @brief 각도의 탄젠트 값을 반환합니다.
     * @param[in] Angle 각도입니다.
     * @return 탄젠트 값입니다.
     */
    template <typename T>
    T Tan(Core::TQuantity<Core::Dim::Angle, T> Angle) {
        return std::tan(Angle.GetBase());
    }

    /**
     * @brief 사인 값에 해당하는 각도를 반환합니다.
     * @param[in] Value [-1, 1] 범위의 사인 값입니다.
     * @return [-90°, 90°] 범위의 각도입니다.
     */
    inline Core::FAngle Asin(float Value) {
        return Core::FAngle::FromBase(std::asin(Value));
    }

    /**
     * @brief 코사인 값에 해당하는 각도를 반환합니다.
     * @param[in] Value [-1, 1] 범위의 코사인 값입니다.
     * @return [0°, 180°] 범위의 각도입니다.
     */
    inline Core::FAngle Acos(float Value) {
        return Core::FAngle::FromBase(std::acos(Value));
    }

    /**
     * @brief 두 성분이 이루는 방향의 각도를 반환합니다.
     * @param[in] Y 세로 성분입니다.
     * @param[in] X 가로 성분입니다.
     * @return (-180°, 180°] 범위의 각도입니다.
     */
    inline Core::FAngle Atan2(float Y, float X) {
        return Core::FAngle::FromBase(std::atan2(Y, X));
    }

    /**
     * @brief 각도를 (-180°, 180°] 범위로 정규화합니다.
     * @param[in] Angle 정규화할 각도입니다.
     * @return 정규화한 각도입니다.
     */
    template <typename T>
    Core::TQuantity<Core::Dim::Angle, T> NormalizeSigned(Core::TQuantity<Core::Dim::Angle, T> Angle) {
        constexpr T Pi = std::numbers::pi_v<T>;
        T Result = std::remainder(Angle.GetBase(), 2 * Pi);
        if (Result <= -Pi) {
            Result += 2 * Pi;
        }
        return Core::TQuantity<Core::Dim::Angle, T>::FromBase(Result);
    }

    /**
     * @brief 각도를 [0°, 360°) 범위로 정규화합니다.
     * @param[in] Angle 정규화할 각도입니다.
     * @return 정규화한 각도입니다.
     */
    template <typename T>
    Core::TQuantity<Core::Dim::Angle, T> NormalizeUnsigned(Core::TQuantity<Core::Dim::Angle, T> Angle) {
        constexpr T TwoPi = 2 * std::numbers::pi_v<T>;
        T Result = std::fmod(Angle.GetBase(), TwoPi);
        if (Result < 0) {
            Result += TwoPi;
        }
        // 아주 작은 음수에 2π를 더하면 반올림으로 2π가 될 수 있습니다.
        if (Result >= TwoPi) {
            Result = 0;
        }
        return Core::TQuantity<Core::Dim::Angle, T>::FromBase(Result);
    }

    /**
     * @brief From에서 To로 가는 가장 짧은 회전 각도를 반환합니다.
     * @param[in] From 시작 각도입니다.
     * @param[in] To 목표 각도입니다.
     * @return (-180°, 180°] 범위의 부호 있는 차이입니다.
     */
    template <typename T>
    Core::TQuantity<Core::Dim::Angle, T> DeltaAngle(Core::TQuantity<Core::Dim::Angle, T> From, Core::TQuantity<Core::Dim::Angle, T> To) {
        return Core::Math::NormalizeSigned(To - From);
    }

    /**
     * @brief 두 각도를 가장 짧은 회전 경로로 보간합니다.
     * @param[in] From 시작 각도입니다.
     * @param[in] To 목표 각도입니다.
     * @param[in] Alpha 0이면 From, 1이면 To 방향이 되는 보간 계수입니다.
     * @return 보간한 각도이며 정규화하지 않습니다.
     */
    template <typename T>
    Core::TQuantity<Core::Dim::Angle, T> LerpAngle(Core::TQuantity<Core::Dim::Angle, T> From, Core::TQuantity<Core::Dim::Angle, T> To, std::type_identity_t<T> Alpha) {
        return From + Core::Math::DeltaAngle(From, To) * Alpha;
    }

    /**
     * @brief 반지름과 중심각으로 호의 길이를 구합니다.
     * @details 각도를 별도 차원으로 두므로 길이와 각도의 곱을 길이로 바꾸는 지점을 이 함수로 명시합니다.
     * @param[in] Radius 원의 반지름입니다.
     * @param[in] Angle 중심각입니다.
     * @return 호의 길이입니다.
     */
    template <typename T>
    constexpr Core::TQuantity<Core::Dim::Length, T> ArcLength(Core::TQuantity<Core::Dim::Length, T> Radius, Core::TQuantity<Core::Dim::Angle, T> Angle) {
        return Core::TQuantity<Core::Dim::Length, T>::FromBase(Radius.GetBase() * Angle.GetBase());
    }

    /**
     * @brief 각도로 FRotator를 만듭니다.
     * @param[in] Pitch Y축 주위 회전각입니다.
     * @param[in] Yaw Z축 주위 회전각입니다.
     * @param[in] Roll X축 주위 회전각입니다.
     * @return 도 단위로 변환한 오일러 회전입니다.
     */
    inline Core::FRotator MakeRotator(Core::FAngle Pitch, Core::FAngle Yaw, Core::FAngle Roll) {
        using namespace Core::Literals;
        return Core::FRotator{Pitch / 1_deg, Yaw / 1_deg, Roll / 1_deg};
    }

    /**
     * @brief FRotator의 Y축 주위 Pitch 회전각을 반환합니다.
     * @param[in] Rotation 오일러 회전입니다.
     * @return Pitch 각도입니다.
     */
    inline Core::FAngle GetPitch(const Core::FRotator& Rotation) {
        using namespace Core::Literals;
        return Rotation.pitch * 1_deg;
    }

    /**
     * @brief FRotator의 Z축 주위 Yaw 회전각을 반환합니다.
     * @param[in] Rotation 오일러 회전입니다.
     * @return Yaw 각도입니다.
     */
    inline Core::FAngle GetYaw(const Core::FRotator& Rotation) {
        using namespace Core::Literals;
        return Rotation.yaw * 1_deg;
    }

    /**
     * @brief FRotator의 X축 주위 Roll 회전각을 반환합니다.
     * @param[in] Rotation 오일러 회전입니다.
     * @return Roll 각도입니다.
     */
    inline Core::FAngle GetRoll(const Core::FRotator& Rotation) {
        using namespace Core::Literals;
        return Rotation.roll * 1_deg;
    }

}
