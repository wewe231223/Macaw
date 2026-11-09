/**
 * @file FNativeType.h
 * @brief 네이티브 타입의 크기, 정렬과 컴파일러 타입 식별 문자열입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <cstddef>
#include <string_view>

namespace CoreUObject {

    /**
     * @brief 네이티브 타입의 크기, 정렬과 컴파일러 타입 식별 문자열입니다.
     * @ingroup CoreUObject
     */
    struct FNativeType {
    public:
        /**
         * @brief 네이티브 타입의 바이트 크기입니다.
         */
        std::size_t mSize{};

        /**
         * @brief 타입 또는 할당에 필요한 바이트 정렬입니다.
         */
        std::size_t mAlignment{};

        /**
         * @brief 네이티브 타입 이름입니다.
         */
        std::string_view mName{};
    };

}
