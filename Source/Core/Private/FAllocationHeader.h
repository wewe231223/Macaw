/**
 * @file FAllocationHeader.h
 * @brief 정렬된 할당 직전에 보관하는 원본 주소와 요청 크기입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <cstddef>

namespace Core {

    /**
     * @brief 정렬된 할당 직전에 보관하는 원본 주소와 요청 크기입니다.
     * @ingroup Core
     */
    struct FAllocationHeader {
    public:
        /**
         * @brief 해제에 사용할 원본 힙 주소입니다.
         */
        void* mRaw{};

        /**
         * @brief 요청한 할당 크기(바이트)입니다.
         */
        std::size_t mSize{};
    };

}
