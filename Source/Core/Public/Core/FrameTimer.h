/**
 * @file FrameTimer.h
 * @brief 프레임 간 경과 시간을 측정합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <chrono>

namespace Core {

    /**
     * @brief 단조 증가 시계를 사용하여 프레임 사이의 경과 시간을 측정합니다.
     * @ingroup Core
     */
    class FFrameTimer {
    public:
        /**
         * @brief FFrameTimer의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        FFrameTimer();

    public:
        /**
         * @brief 이전 호출 이후 경과한 시간을 초 단위로 반환하고 기준 시각을 갱신합니다.
         * @return 초 단위 경과 시간입니다.
         */
        float Tick();

    private:
        /**
         * @brief 이전 프레임 기준 시각입니다.
         */
        std::chrono::steady_clock::time_point mPrevious{};
    };

}
