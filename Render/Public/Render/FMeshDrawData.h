#pragma once

#include "Render/FMeshDrawState.h"

struct FMeshDrawRecord {
    Uint32 mObjectIndex{};
    Uint32 mMaterialIndex{};
    Uint32 mFlags{};
    float mLODDither{};
};

static_assert(sizeof(FMeshDrawRecord) == 16);

struct FMeshDrawBatch {
    FMeshDrawState mState{};
    Uint32 mFirstRecord{};
    Uint32 mRecordCount{};
    Uint32 mFlags{};
    float mSortDepth{};
};
