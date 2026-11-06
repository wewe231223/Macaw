/**
 * @file EMeshPass.h
 * @brief 메시 패스의 종류를 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "EMeshPass.generated.h"

namespace Renderer {

    /**
     * @brief 입력 레이아웃과 패스 전후 바인딩 정책을 결정하는 메시 패스 종류입니다.
     */
    UENUM()
    enum class EMeshPass : Core::Uint8 {
        /**
         * @brief 그림자를 드리우는 메시의 깊이만 그림자 아틀라스 타일에 기록합니다.
         */
        ShadowDepth,
        /**
         * @brief 보이는 메시를 머티리얼과 조명으로 장면 색상·깊이 타깃에 그립니다.
         */
        BasePass
    };

}
