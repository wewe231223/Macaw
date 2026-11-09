/**
 * @file EWorldPlayState.h
 * @brief 월드의 게임 실행 생명주기 상태를 구분합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "EWorldPlayState.generated.h"

namespace Engine {

    /**
     * @brief 월드의 게임 실행 생명주기 상태를 구분합니다.
     * @ingroup Engine
     */
    UENUM()
    enum class EWorldPlayState : Core::Uint8 {
        /**
         * @brief 게임 생명주기가 시작되지 않았거나 종료된 상태입니다.
         */
        Stopped,
        /**
         * @brief 액터와 컴포넌트에 시작 콜백을 전달하는 상태입니다.
         */
        Starting,
        /**
         * @brief 일반 Tick으로 게임 시간을 진행하는 상태입니다.
         */
        Playing,
        /**
         * @brief 일반 Tick을 중지하고 Step으로만 시간을 진행하는 상태입니다.
         */
        Paused,
        /**
         * @brief 시작된 액터와 컴포넌트에 종료 콜백을 전달하는 상태입니다.
         */
        Stopping
    };

}
