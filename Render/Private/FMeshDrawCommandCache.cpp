#include "pch.h"
#include "Render/FMeshDrawCommandCache.h"

#include <tuple>

bool operator<(const FMeshDrawCommandKey& Left, const FMeshDrawCommandKey& Right) {
    return std::tie(Left.mMeshHandle.mId, Left.mMeshHandle.mGeneration, Left.mMaterialHandle.mId, Left.mMaterialHandle.mGeneration, Left.mPipelineHandle.mId, Left.mPipelineHandle.mGeneration, Left.mLODLevel, Left.mFirstIndex, Left.mIndexCount, Left.mOriginalIndexCount, Left.mMaterialIndex, Left.mPass, Left.mGPUResources) < std::tie(Right.mMeshHandle.mId, Right.mMeshHandle.mGeneration, Right.mMaterialHandle.mId, Right.mMaterialHandle.mGeneration, Right.mPipelineHandle.mId, Right.mPipelineHandle.mGeneration, Right.mLODLevel, Right.mFirstIndex, Right.mIndexCount, Right.mOriginalIndexCount, Right.mMaterialIndex, Right.mPass, Right.mGPUResources);
}

void FMeshDrawCommandCache::BeginUpdate() {
    ++mUpdateSerial;
    mCommands.clear();
}

void FMeshDrawCommandCache::EndUpdate() {
    std::erase_if(mEntries, [this](const auto& Entry) {
        return Entry.second.mLastUsedUpdate != mUpdateSerial;
    });
}

const TArray<std::shared_ptr<const FMeshDrawCommand>>& FMeshDrawCommandCache::GetCommands() const {
    return mCommands;
}
