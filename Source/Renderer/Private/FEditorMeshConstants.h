#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"
#include <array>
#include <cstddef>
#include <type_traits>

struct FEditorMeshConstants {
public:
    ReflectTypes(FEditorMeshConstants);

public:
    ReflectStruct(FEditorMeshConstants);

private:
    ReflectMembers();

public:
    FMatrix mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<float, 4> mColor{};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

static_assert(std::is_standard_layout_v<FEditorMeshConstants> && std::is_trivially_copyable_v<FEditorMeshConstants> && sizeof(FEditorMeshConstants) == 80);
static_assert(offsetof(FEditorMeshConstants, mWorld) == 0 && offsetof(FEditorMeshConstants, mColor) == 64);
