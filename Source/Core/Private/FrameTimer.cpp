/**
 * @file FrameTimer.cpp
 * @brief 프레임 간 경과 시간을 측정합니다.
 * @ingroup Core
 */

#include "Core/FrameTimer.h"

#include <algorithm>

/**
 * @brief FFrameTimer의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FFrameTimer::FFrameTimer()
	: mPrevious{std::chrono::steady_clock::now()} {
}

/**
 * @brief 이전 호출 이후 경과한 시간을 초 단위로 반환하고 기준 시각을 갱신합니다.
 */
float FFrameTimer::Tick() {
    const auto Current{std::chrono::steady_clock::now()};
    const float Delta{std::chrono::duration<float>{Current - mPrevious}.count()};
    mPrevious = Current;

    return std::clamp(Delta, 0.000001f, 0.1f);
}
