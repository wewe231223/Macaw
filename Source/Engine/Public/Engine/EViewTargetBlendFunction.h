/**
 * @file EViewTargetBlendFunction.h
 * @brief 뷰 타깃을 전환할 때 사용하는 블렌드 곡선입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "EViewTargetBlendFunction.generated.h"

namespace Engine {

    /**
     * @brief 뷰 타깃을 전환할 때 사용하는 블렌드 곡선입니다.
     * @ingroup Engine
     */
    UENUM(Script)
    enum class EViewTargetBlendFunction : Core::Uint8 {
        /**
         * @brief 일정한 속도로 보간합니다.
         */
        Linear,

        /**
         * @brief 시작과 끝에서 느려지는 3차 곡선으로 보간합니다.
         */
        Cubic,

        /**
         * @brief 처음에 느리게 출발합니다.
         */
        EaseIn,

        /**
         * @brief 끝에서 느리게 멈춥니다.
         */
        EaseOut
    };

}
