#pragma once

#include "Core/ReflectionMacros.h"

struct FRotator {
public:
    ReflectTypes(FRotator);

public:
    ReflectStruct(FRotator);

private:
    ReflectMembers();

public:
    float mPitch{};
    float mYaw{};
    float mRoll{};
};
