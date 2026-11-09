/**
 * @file EPrimitiveType.h
 * @brief 기본 생성 및 렌더링에 지원하는 프리미티브 메시 종류입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "EPrimitiveType.generated.h"

namespace Engine {

    /**
     * @brief 기본 생성 및 렌더링에 지원하는 프리미티브 메시 종류입니다.
     * @ingroup Engine
     */
    UENUM(Script)
    enum class EPrimitiveType : Core::Uint8 {
        /**
         * @brief 기본 평면 메시입니다.
         */
        Plane,
        /**
         * @brief 기본 정육면체 메시입니다.
         */
        Cube,
        /**
         * @brief 기본 구 메시입니다.
         */
        Sphere,
        /**
         * @brief 지원하는 프리미티브 종류의 개수이며 실제 메시 종류가 아닙니다.
         */
        Count UMETA(Hidden)
    };

}
