/**
 * @file FMemoryStats.h
 * @brief 객체 개수와 추적 중인 힙 사용량의 스냅샷입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <cstddef>

namespace Core {

    /**
     * @brief 객체 개수와 추적 중인 힙 사용량의 스냅샷입니다.
     * @ingroup Core
     */
    struct FMemoryStats {
    public:
        /**
         * @brief 현재 생존 중인 UObject 개수입니다.
         */
        std::size_t mLiveObjects{};

        /**
         * @brief 지금까지 생성한 UObject 개수입니다.
         */
        std::size_t mCreatedObjects{};

        /**
         * @brief 현재 추적 중인 힙 할당 바이트 수입니다.
         */
        std::size_t mHeapBytes{};

        /**
         * @brief 추적 중인 힙 사용량의 최댓값입니다.
         */
        std::size_t mPeakHeapBytes{};
    };

}
