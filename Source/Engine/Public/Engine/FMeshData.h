#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FMeshVertex.h"
#include "Core/Math/FBox.h"

struct FMeshData {
public:
    ReflectTypes(FMeshData);

public:
    ReflectStruct(FMeshData);

private:
    ReflectMembers();

public:
    TArray<FMeshVertex> mVertices{};
    ReflectProperty(mVertices, "Vertices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    TArray<Uint32> mIndices{};
    ReflectProperty(mIndices, "Indices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FBox mBounds{};
    ReflectProperty(mBounds, "Bounds", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
