#include "Core/FrameTimer.h"

#include <algorithm>

FFrameTimer::FFrameTimer()
	: mPrevious{std::chrono::steady_clock::now()} {
}

float FFrameTimer::Tick() {
    const auto Current{std::chrono::steady_clock::now()};
    const float Delta{std::chrono::duration<float>{Current - mPrevious}.count()};
    mPrevious = Current;

    return std::clamp(Delta, 0.000001f, 0.1f);
}
