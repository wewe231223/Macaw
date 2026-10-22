#pragma once

#include "Core/ReflectionMacros.h"

#include <array>
#include <cstddef>
#include <type_traits>

struct FVector;

struct FLineVertex {
public:
    ReflectTypes(FLineVertex);

public:
    FLineVertex(const FVector& Position, const FVector& Color);

private:
    ReflectMembers();

public:
    ReflectStruct(FLineVertex);

    std::array<float, 3> mPosition{};
    ReflectProperty(mPosition, "Position", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<float, 3> mColor{};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

static_assert(std::is_standard_layout_v<FLineVertex> && std::is_trivially_copyable_v<FLineVertex>);
static_assert(sizeof(FLineVertex) == 24 && offsetof(FLineVertex, mPosition) == 0 && offsetof(FLineVertex, mColor) == 12);
