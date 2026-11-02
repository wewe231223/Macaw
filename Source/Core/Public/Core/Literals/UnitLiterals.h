/**
 * @file UnitLiterals.h
 * @brief 실세계 단위로 물리량을 만드는 사용자 정의 리터럴을 정의합니다.
 * @details 사용하는 범위에서 using namespace Core::Literals;로 가져옵니다.
 *          모든 리터럴은 부동소수(1.5_m)와 정수(90_deg) 표기를 모두 받습니다.
 * @ingroup Core
 */

#pragma once

#include "Core/Literals/QuantityTypes.h"

#include <numbers>

namespace Core::Literals {

    namespace Detail {
        /**
         * @brief 단위 변환 계수 계산에 쓰는 원주율입니다.
         */
        inline constexpr long double kPi = std::numbers::pi_v<long double>;

        /**
         * @brief 리터럴 값에 단위 계수를 곱해 SI 기본 단위 물리량을 만듭니다.
         * @tparam Q 만들 물리량 타입입니다.
         * @param[in] Value 리터럴에 적힌 값입니다.
         * @param[in] Scale 단위 하나를 SI 기본 단위로 표현한 계수입니다.
         * @return 생성한 물리량입니다.
         */
        template <typename Q>
        constexpr Q MakeUnit(long double Value, long double Scale) {
            return Q::FromBase(static_cast<typename Q::ValueType>(Value * Scale));
        }
    }

    /**
     * @name 길이 리터럴
     * @{
     */

    /** @brief 밀리미터(mm) 값을 FLength로 만듭니다. */
    constexpr Core::FLength operator""_mm(long double Value) { return Detail::MakeUnit<Core::FLength>(Value, 0.001L); }
    /** @brief 밀리미터(mm) 값을 FLength로 만듭니다. */
    constexpr Core::FLength operator""_mm(unsigned long long Value) { return Detail::MakeUnit<Core::FLength>(static_cast<long double>(Value), 0.001L); }

    /** @brief 센티미터(cm) 값을 FLength로 만듭니다. */
    constexpr Core::FLength operator""_cm(long double Value) { return Detail::MakeUnit<Core::FLength>(Value, 0.01L); }
    /** @brief 센티미터(cm) 값을 FLength로 만듭니다. */
    constexpr Core::FLength operator""_cm(unsigned long long Value) { return Detail::MakeUnit<Core::FLength>(static_cast<long double>(Value), 0.01L); }

    /** @brief 미터(m) 값을 FLength로 만듭니다. */
    constexpr Core::FLength operator""_m(long double Value) { return Detail::MakeUnit<Core::FLength>(Value, 1.0L); }
    /** @brief 미터(m) 값을 FLength로 만듭니다. */
    constexpr Core::FLength operator""_m(unsigned long long Value) { return Detail::MakeUnit<Core::FLength>(static_cast<long double>(Value), 1.0L); }

    /** @brief 킬로미터(km) 값을 FLength로 만듭니다. */
    constexpr Core::FLength operator""_km(long double Value) { return Detail::MakeUnit<Core::FLength>(Value, 1000.0L); }
    /** @brief 킬로미터(km) 값을 FLength로 만듭니다. */
    constexpr Core::FLength operator""_km(unsigned long long Value) { return Detail::MakeUnit<Core::FLength>(static_cast<long double>(Value), 1000.0L); }

    /** @} */

    /**
     * @name 질량 리터럴
     * @{
     */

    /** @brief 그램(g) 값을 FMass로 만듭니다. */
    constexpr Core::FMass operator""_g(long double Value) { return Detail::MakeUnit<Core::FMass>(Value, 0.001L); }
    /** @brief 그램(g) 값을 FMass로 만듭니다. */
    constexpr Core::FMass operator""_g(unsigned long long Value) { return Detail::MakeUnit<Core::FMass>(static_cast<long double>(Value), 0.001L); }

    /** @brief 킬로그램(kg) 값을 FMass로 만듭니다. */
    constexpr Core::FMass operator""_kg(long double Value) { return Detail::MakeUnit<Core::FMass>(Value, 1.0L); }
    /** @brief 킬로그램(kg) 값을 FMass로 만듭니다. */
    constexpr Core::FMass operator""_kg(unsigned long long Value) { return Detail::MakeUnit<Core::FMass>(static_cast<long double>(Value), 1.0L); }

    /** @brief 톤(t) 값을 FMass로 만듭니다. */
    constexpr Core::FMass operator""_t(long double Value) { return Detail::MakeUnit<Core::FMass>(Value, 1000.0L); }
    /** @brief 톤(t) 값을 FMass로 만듭니다. */
    constexpr Core::FMass operator""_t(unsigned long long Value) { return Detail::MakeUnit<Core::FMass>(static_cast<long double>(Value), 1000.0L); }

    /** @} */

    /**
     * @name 시간 리터럴
     * @{
     */

    /** @brief 밀리초(ms) 값을 FTime으로 만듭니다. */
    constexpr Core::FTime operator""_ms(long double Value) { return Detail::MakeUnit<Core::FTime>(Value, 0.001L); }
    /** @brief 밀리초(ms) 값을 FTime으로 만듭니다. */
    constexpr Core::FTime operator""_ms(unsigned long long Value) { return Detail::MakeUnit<Core::FTime>(static_cast<long double>(Value), 0.001L); }

    /** @brief 초(s) 값을 FTime으로 만듭니다. */
    constexpr Core::FTime operator""_s(long double Value) { return Detail::MakeUnit<Core::FTime>(Value, 1.0L); }
    /** @brief 초(s) 값을 FTime으로 만듭니다. */
    constexpr Core::FTime operator""_s(unsigned long long Value) { return Detail::MakeUnit<Core::FTime>(static_cast<long double>(Value), 1.0L); }

    /** @brief 분(min) 값을 FTime으로 만듭니다. */
    constexpr Core::FTime operator""_min(long double Value) { return Detail::MakeUnit<Core::FTime>(Value, 60.0L); }
    /** @brief 분(min) 값을 FTime으로 만듭니다. */
    constexpr Core::FTime operator""_min(unsigned long long Value) { return Detail::MakeUnit<Core::FTime>(static_cast<long double>(Value), 60.0L); }

    /** @brief 시간(h) 값을 FTime으로 만듭니다. */
    constexpr Core::FTime operator""_h(long double Value) { return Detail::MakeUnit<Core::FTime>(Value, 3600.0L); }
    /** @brief 시간(h) 값을 FTime으로 만듭니다. */
    constexpr Core::FTime operator""_h(unsigned long long Value) { return Detail::MakeUnit<Core::FTime>(static_cast<long double>(Value), 3600.0L); }

    /** @} */

    /**
     * @name 속도와 가속도 리터럴
     * @{
     */

    /** @brief 초속 미터(m/s) 값을 FSpeed로 만듭니다. */
    constexpr Core::FSpeed operator""_mps(long double Value) { return Detail::MakeUnit<Core::FSpeed>(Value, 1.0L); }
    /** @brief 초속 미터(m/s) 값을 FSpeed로 만듭니다. */
    constexpr Core::FSpeed operator""_mps(unsigned long long Value) { return Detail::MakeUnit<Core::FSpeed>(static_cast<long double>(Value), 1.0L); }

    /** @brief 시속 킬로미터(km/h) 값을 FSpeed로 만듭니다. */
    constexpr Core::FSpeed operator""_kmh(long double Value) { return Detail::MakeUnit<Core::FSpeed>(Value, 1000.0L / 3600.0L); }
    /** @brief 시속 킬로미터(km/h) 값을 FSpeed로 만듭니다. */
    constexpr Core::FSpeed operator""_kmh(unsigned long long Value) { return Detail::MakeUnit<Core::FSpeed>(static_cast<long double>(Value), 1000.0L / 3600.0L); }

    /** @brief 미터 매 초 제곱(m/s²) 값을 FAcceleration으로 만듭니다. */
    constexpr Core::FAcceleration operator""_mps2(long double Value) { return Detail::MakeUnit<Core::FAcceleration>(Value, 1.0L); }
    /** @brief 미터 매 초 제곱(m/s²) 값을 FAcceleration으로 만듭니다. */
    constexpr Core::FAcceleration operator""_mps2(unsigned long long Value) { return Detail::MakeUnit<Core::FAcceleration>(static_cast<long double>(Value), 1.0L); }

    /** @} */

    /**
     * @name 힘, 에너지, 일률, 주파수 리터럴
     * @{
     */

    /** @brief 뉴턴(N) 값을 FForce로 만듭니다. */
    constexpr Core::FForce operator""_N(long double Value) { return Detail::MakeUnit<Core::FForce>(Value, 1.0L); }
    /** @brief 뉴턴(N) 값을 FForce로 만듭니다. */
    constexpr Core::FForce operator""_N(unsigned long long Value) { return Detail::MakeUnit<Core::FForce>(static_cast<long double>(Value), 1.0L); }

    /** @brief 킬로뉴턴(kN) 값을 FForce로 만듭니다. */
    constexpr Core::FForce operator""_kN(long double Value) { return Detail::MakeUnit<Core::FForce>(Value, 1000.0L); }
    /** @brief 킬로뉴턴(kN) 값을 FForce로 만듭니다. */
    constexpr Core::FForce operator""_kN(unsigned long long Value) { return Detail::MakeUnit<Core::FForce>(static_cast<long double>(Value), 1000.0L); }

    /** @brief 줄(J) 값을 FEnergy로 만듭니다. */
    constexpr Core::FEnergy operator""_J(long double Value) { return Detail::MakeUnit<Core::FEnergy>(Value, 1.0L); }
    /** @brief 줄(J) 값을 FEnergy로 만듭니다. */
    constexpr Core::FEnergy operator""_J(unsigned long long Value) { return Detail::MakeUnit<Core::FEnergy>(static_cast<long double>(Value), 1.0L); }

    /** @brief 킬로줄(kJ) 값을 FEnergy로 만듭니다. */
    constexpr Core::FEnergy operator""_kJ(long double Value) { return Detail::MakeUnit<Core::FEnergy>(Value, 1000.0L); }
    /** @brief 킬로줄(kJ) 값을 FEnergy로 만듭니다. */
    constexpr Core::FEnergy operator""_kJ(unsigned long long Value) { return Detail::MakeUnit<Core::FEnergy>(static_cast<long double>(Value), 1000.0L); }

    /** @brief 와트(W) 값을 FPower로 만듭니다. */
    constexpr Core::FPower operator""_W(long double Value) { return Detail::MakeUnit<Core::FPower>(Value, 1.0L); }
    /** @brief 와트(W) 값을 FPower로 만듭니다. */
    constexpr Core::FPower operator""_W(unsigned long long Value) { return Detail::MakeUnit<Core::FPower>(static_cast<long double>(Value), 1.0L); }

    /** @brief 킬로와트(kW) 값을 FPower로 만듭니다. */
    constexpr Core::FPower operator""_kW(long double Value) { return Detail::MakeUnit<Core::FPower>(Value, 1000.0L); }
    /** @brief 킬로와트(kW) 값을 FPower로 만듭니다. */
    constexpr Core::FPower operator""_kW(unsigned long long Value) { return Detail::MakeUnit<Core::FPower>(static_cast<long double>(Value), 1000.0L); }

    /** @brief 헤르츠(Hz) 값을 FFrequency로 만듭니다. */
    constexpr Core::FFrequency operator""_Hz(long double Value) { return Detail::MakeUnit<Core::FFrequency>(Value, 1.0L); }
    /** @brief 헤르츠(Hz) 값을 FFrequency로 만듭니다. */
    constexpr Core::FFrequency operator""_Hz(unsigned long long Value) { return Detail::MakeUnit<Core::FFrequency>(static_cast<long double>(Value), 1.0L); }

    /** @} */

    /**
     * @name 각도 리터럴
     * @{
     */

    /** @brief 라디안(rad) 값을 FAngle로 만듭니다. */
    constexpr Core::FAngle operator""_rad(long double Value) { return Detail::MakeUnit<Core::FAngle>(Value, 1.0L); }
    /** @brief 라디안(rad) 값을 FAngle로 만듭니다. */
    constexpr Core::FAngle operator""_rad(unsigned long long Value) { return Detail::MakeUnit<Core::FAngle>(static_cast<long double>(Value), 1.0L); }

    /** @brief 도(°) 값을 FAngle로 만듭니다. */
    constexpr Core::FAngle operator""_deg(long double Value) { return Detail::MakeUnit<Core::FAngle>(Value, Detail::kPi / 180.0L); }
    /** @brief 도(°) 값을 FAngle로 만듭니다. */
    constexpr Core::FAngle operator""_deg(unsigned long long Value) { return Detail::MakeUnit<Core::FAngle>(static_cast<long double>(Value), Detail::kPi / 180.0L); }

    /** @brief 회전수(한 바퀴 = 360°) 값을 FAngle로 만듭니다. */
    constexpr Core::FAngle operator""_turn(long double Value) { return Detail::MakeUnit<Core::FAngle>(Value, 2.0L * Detail::kPi); }
    /** @brief 회전수(한 바퀴 = 360°) 값을 FAngle로 만듭니다. */
    constexpr Core::FAngle operator""_turn(unsigned long long Value) { return Detail::MakeUnit<Core::FAngle>(static_cast<long double>(Value), 2.0L * Detail::kPi); }

    /** @brief 라디안 매 초(rad/s) 값을 FAngularSpeed로 만듭니다. */
    constexpr Core::FAngularSpeed operator""_radps(long double Value) { return Detail::MakeUnit<Core::FAngularSpeed>(Value, 1.0L); }
    /** @brief 라디안 매 초(rad/s) 값을 FAngularSpeed로 만듭니다. */
    constexpr Core::FAngularSpeed operator""_radps(unsigned long long Value) { return Detail::MakeUnit<Core::FAngularSpeed>(static_cast<long double>(Value), 1.0L); }

    /** @brief 도 매 초(°/s) 값을 FAngularSpeed로 만듭니다. */
    constexpr Core::FAngularSpeed operator""_degps(long double Value) { return Detail::MakeUnit<Core::FAngularSpeed>(Value, Detail::kPi / 180.0L); }
    /** @brief 도 매 초(°/s) 값을 FAngularSpeed로 만듭니다. */
    constexpr Core::FAngularSpeed operator""_degps(unsigned long long Value) { return Detail::MakeUnit<Core::FAngularSpeed>(static_cast<long double>(Value), Detail::kPi / 180.0L); }

    /** @brief 분당 회전수(rpm) 값을 FAngularSpeed로 만듭니다. */
    constexpr Core::FAngularSpeed operator""_rpm(long double Value) { return Detail::MakeUnit<Core::FAngularSpeed>(Value, 2.0L * Detail::kPi / 60.0L); }
    /** @brief 분당 회전수(rpm) 값을 FAngularSpeed로 만듭니다. */
    constexpr Core::FAngularSpeed operator""_rpm(unsigned long long Value) { return Detail::MakeUnit<Core::FAngularSpeed>(static_cast<long double>(Value), 2.0L * Detail::kPi / 60.0L); }

    /** @brief 라디안 매 초 제곱(rad/s²) 값을 FAngularAcceleration으로 만듭니다. */
    constexpr Core::FAngularAcceleration operator""_radps2(long double Value) { return Detail::MakeUnit<Core::FAngularAcceleration>(Value, 1.0L); }
    /** @brief 라디안 매 초 제곱(rad/s²) 값을 FAngularAcceleration으로 만듭니다. */
    constexpr Core::FAngularAcceleration operator""_radps2(unsigned long long Value) { return Detail::MakeUnit<Core::FAngularAcceleration>(static_cast<long double>(Value), 1.0L); }

    /** @brief 도 매 초 제곱(°/s²) 값을 FAngularAcceleration으로 만듭니다. */
    constexpr Core::FAngularAcceleration operator""_degps2(long double Value) { return Detail::MakeUnit<Core::FAngularAcceleration>(Value, Detail::kPi / 180.0L); }
    /** @brief 도 매 초 제곱(°/s²) 값을 FAngularAcceleration으로 만듭니다. */
    constexpr Core::FAngularAcceleration operator""_degps2(unsigned long long Value) { return Detail::MakeUnit<Core::FAngularAcceleration>(static_cast<long double>(Value), Detail::kPi / 180.0L); }

    /** @} */

    static_assert(1_km == 1000_m && 100_cm == 1_m);
    static_assert(2_h == 120_min);
    static_assert(36_kmh == 10_mps);
    static_assert(1_kg * 1_mps2 == 1_N);
    static_assert(1_N * 1_m / 1_s == 1_W);
    static_assert(1_km / 1_m == 1000.0f);

}
