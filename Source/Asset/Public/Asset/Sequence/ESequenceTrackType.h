/**
 * @file ESequenceTrackType.h
 * @brief 시퀀스 트랙의 구체 타입을 구분합니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

namespace Asset {

    /**
     * @brief 시퀀스 트랙의 구체 타입을 구분합니다.
     * @ingroup Asset
     */
    enum class ESequenceTrackType : Core::Uint8 {
        /**
         * @brief 바인딩 대상의 변환을 제어하는 트랙입니다.
         */
        Transform,
        /**
         * @brief 바인딩 대상의 float 프로퍼티를 제어하는 트랙입니다.
         */
        Property
    };

    ReflectEnum(ESequenceTrackType, Transform, Property);

}
