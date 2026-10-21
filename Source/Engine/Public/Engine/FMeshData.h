#pragma once

#include "Engine/FMeshVertex.h"
#include "Core/Math/FBox.h"

struct FMeshData {
    TArray<FMeshVertex> mVertices{};
    TArray<Uint32> mIndices{};
    FBox mBounds{};
};
