/**
 * @file ESequenceInterpolation.h
 * @brief 시퀀스 키에서 다음 키까지의 보간 방식을 구분합니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

namespace Asset {

    /**
     * @brief 시퀀스 키에서 다음 키까지의 보간 방식을 구분합니다.
     * @ingroup Asset
     */
    enum class ESequenceInterpolation : Core::Uint8 {
        /**
         * @brief 다음 키까지 현재 키 값을 유지합니다.
         */
        Constant,
        /**
         * @brief 다음 키까지 일정한 속도로 변화합니다.
         */
        Linear,
        /**
         * @brief 주변 키에서 계산한 자동 탄젠트로 부드럽게 변화합니다.
         */
        Cubic
    };

    ReflectEnum(ESequenceInterpolation, Constant, Linear, Cubic);

}
