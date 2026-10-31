/**
 * @file EMeshPass.h
 * @brief 메시 패스의 종류를 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

namespace Renderer {

    /**
     * @brief 입력 레이아웃과 패스 전후 바인딩 정책을 결정하는 메시 패스 종류입니다.
     */
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

    /**
     * @brief 해당 열거형의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectEnum(EMeshPass, ShadowDepth, BasePass);

}
