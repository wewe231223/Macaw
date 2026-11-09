/**
 * @file FPropertyChangedEvent.h
 * @brief 에디터에서 변경한 프로퍼티와 연속 편집 여부를 전달합니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"

namespace CoreUObject {

    class FProperty;

    /**
     * @brief 에디터에서 변경한 프로퍼티와 연속 편집 여부를 전달합니다.
     * @ingroup CoreUObject
     */
    struct FPropertyChangedEvent {
    public:
        /**
         * @brief 변경한 프로퍼티의 비소유 주소입니다.
         */
        const CoreUObject::FProperty* mProperty{};

        /**
         * @brief 드래그 등 연속 편집 중인지 여부입니다.
         */
        bool mInteractive{};
    };

}
