#pragma once

#include <chrono>

class FFrameTimer {
public:
    FFrameTimer();

public:
    float Tick();

private:
    std::chrono::steady_clock::time_point mPrevious{};
};
