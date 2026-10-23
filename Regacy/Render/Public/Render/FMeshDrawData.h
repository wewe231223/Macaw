#pragma once

#include "Render/FMeshDrawState.h"

struct FMeshDrawRecord {
    Uint32 mObjectIndex{};
    Uint32 mMaterialIndex{};
    float mLODDither{};
    Uint32 mUVTransformIndex{};
};

static_assert(sizeof(FMeshDrawRecord) == 16);

struct FMeshDrawBatch {
    FMeshDrawState mState{};
    Uint32 mFirstRecord{};
    Uint32 mRecordCount{};
    float mSortDepth{};
};
