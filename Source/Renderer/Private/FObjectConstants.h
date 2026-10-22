#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"

#include <cstddef>
#include <type_traits>

struct FObjectConstants {
public:
    ReflectTypes(FObjectConstants);

public:
    ReflectStruct(FObjectConstants);

private:
    ReflectMembers();

public:
    FMatrix mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FMatrix mWorldInverse{};
    ReflectProperty(mWorldInverse, "WorldInverse", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::array<float, 4> mColor{};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

static_assert(sizeof(FMatrix) == 64 && alignof(FMatrix) == 16 && std::is_trivially_copyable_v<FMatrix>);
static_assert(std::is_standard_layout_v<FObjectConstants> && sizeof(FObjectConstants) == 144);
static_assert(offsetof(FObjectConstants, mWorld) == 0 && offsetof(FObjectConstants, mWorldInverse) == 64 && offsetof(FObjectConstants, mColor) == 128);
