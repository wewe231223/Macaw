/**
 * @file ESequencePlayState.h
 * @brief 시퀀스 플레이어의 재생 상태를 구분합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

namespace Engine {

    /**
     * @brief 시퀀스 플레이어의 재생 상태를 구분합니다.
     * @ingroup Engine
     */
    enum class ESequencePlayState : Core::Uint8 {
        /**
         * @brief 재생하지 않으며 다음 재생을 시작 시간부터 진행합니다.
         */
        Stopped,
        /**
         * @brief 시간을 진행하며 값을 적용합니다.
         */
        Playing,
        /**
         * @brief 현재 시간을 유지합니다.
         */
        Paused
    };

    ReflectEnum(ESequencePlayState, Stopped, Playing, Paused);

}
