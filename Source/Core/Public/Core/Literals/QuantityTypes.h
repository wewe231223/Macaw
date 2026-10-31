/**
 * @file QuantityTypes.h
 * @brief 자주 쓰는 물리량의 float 기반 타입 별칭을 정의합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/Literals/TQuantity.h"

namespace Core {

    /**
     * @brief 무차원 비율입니다.
     */
    using FScalar = Core::TQuantity<Core::Dim::None, float>;

    /**
     * @brief 길이(m)입니다.
     */
    using FLength = Core::TQuantity<Core::Dim::Length, float>;

    /**
     * @brief 질량(kg)입니다.
     */
    using FMass = Core::TQuantity<Core::Dim::Mass, float>;

    /**
     * @brief 시간(s)입니다.
     */
    using FTime = Core::TQuantity<Core::Dim::Time, float>;

    /**
     * @brief 각도(rad)입니다.
     */
    using FAngle = Core::TQuantity<Core::Dim::Angle, float>;

    /**
     * @brief 넓이(m²)입니다.
     */
    using FArea = Core::TQuantity<Core::Dim::Area, float>;

    /**
     * @brief 부피(m³)입니다.
     */
    using FVolume = Core::TQuantity<Core::Dim::Volume, float>;

    /**
     * @brief 주파수(Hz)입니다.
     */
    using FFrequency = Core::TQuantity<Core::Dim::Frequency, float>;

    /**
     * @brief 속도(m/s)입니다.
     */
    using FSpeed = Core::TQuantity<Core::Dim::Speed, float>;

    /**
     * @brief 가속도(m/s²)입니다.
     */
    using FAcceleration = Core::TQuantity<Core::Dim::Acceleration, float>;

    /**
     * @brief 힘(N)입니다.
     */
    using FForce = Core::TQuantity<Core::Dim::Force, float>;

    /**
     * @brief 에너지(J)입니다.
     */
    using FEnergy = Core::TQuantity<Core::Dim::Energy, float>;

    /**
     * @brief 일률(W)입니다.
     */
    using FPower = Core::TQuantity<Core::Dim::Power, float>;

    /**
     * @brief 각속도(rad/s)입니다.
     */
    using FAngularSpeed = Core::TQuantity<Core::Dim::AngularSpeed, float>;

    /**
     * @brief 각가속도(rad/s²)입니다.
     */
    using FAngularAcceleration = Core::TQuantity<Core::Dim::AngularAcceleration, float>;

    static_assert(sizeof(Core::FLength) == sizeof(float) && std::is_trivially_copyable_v<Core::FLength>);

}
