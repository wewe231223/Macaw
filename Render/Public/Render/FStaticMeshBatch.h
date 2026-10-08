#pragma once
#include "RenderCore/FMeshBatch.h"

struct FStaticMeshBatch : FMeshBatch {
    Uint32 mObjectIndex{};
};
