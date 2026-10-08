#pragma once
#include "Core/CoreMinimal.h"
#include "Core/Base/FAssetHandle.h"

struct FMeshBatchElement {
    Uint32 mFirstIndex{};
    Uint32 mIndexCount{};
    Uint32 mMaterialGroupIndex{};
    Uint32 mOriginalIndexCount{};
};

struct FMeshBatch {
    FAssetHandle mMeshHandle{};
    FAssetHandle mMaterialHandle{};
    FAssetHandle mPipelineHandle{};
    Uint32 mLODLevel{};
    TArray<FMeshBatchElement> mElements{};
};
