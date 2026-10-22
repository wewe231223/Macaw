#pragma once

#include "Core/ReflectionMacros.h"

#include <array>
#include <cstddef>
#include <type_traits>

struct FMeshVertex;

struct FGpuMeshVertex {
public:
    ReflectTypes(FGpuMeshVertex);

public:
    explicit FGpuMeshVertex(const FMeshVertex& Vertex);

private:
    ReflectMembers();

public:
    ReflectStruct(FGpuMeshVertex);

    std::array<float, 3> mPosition{};
    ReflectProperty(mPosition, "Position", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<float, 3> mNormal{};
    ReflectProperty(mNormal, "Normal", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

static_assert(std::is_standard_layout_v<FGpuMeshVertex> && std::is_trivially_copyable_v<FGpuMeshVertex>);
static_assert(sizeof(FGpuMeshVertex) == 24 && offsetof(FGpuMeshVertex, mPosition) == 0 && offsetof(FGpuMeshVertex, mNormal) == 12);
