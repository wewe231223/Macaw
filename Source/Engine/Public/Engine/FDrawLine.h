#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"

struct FDrawLine {
public:
    ReflectTypes(FDrawLine);

public:
    ReflectStruct(FDrawLine);

private:
    ReflectMembers();

public:
    FVector mStart{};
    ReflectProperty(mStart, "Start", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FVector mEnd{};
    ReflectProperty(mEnd, "End", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FVector mColor{1.0f, 1.0f, 1.0f};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mDepthTest{};
    ReflectProperty(mDepthTest, "DepthTest", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
