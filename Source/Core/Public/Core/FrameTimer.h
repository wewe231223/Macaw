#pragma once

#include "Core/ReflectionMacros.h"

#include <chrono>

class FFrameTimer {
public:
    ReflectTypes(FFrameTimer);

public:
    FFrameTimer();

public:
    ReflectStruct(FFrameTimer);

    float Tick();

private:
    ReflectMembers();

private:
    std::chrono::steady_clock::time_point mPrevious{};
    ReflectProperty(mPrevious, "Previous", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
