/**
 * @file EWorldPlayState.h
 * @brief 월드의 게임 실행 생명주기 상태를 구분합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

/**
 * @brief 월드의 게임 실행 생명주기 상태를 구분합니다.
 * @ingroup Engine
 */
enum class EWorldPlayState : Uint8 {
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

/**
 * @brief 이 열거형의 정적 메타데이터를 반환합니다.
 * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
 */
ReflectEnum(EWorldPlayState, Stopped, Starting, Playing, Paused, Stopping);

/**
 * @fn UEnum* GetNativeEnum(TNativeEnum<EWorldPlayState>)
 * @brief 타입 태그를 통해 이 열거형의 메타데이터를 조회합니다.
 * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
 */
