#include "pch.h"
#include "RenderCore/FSceneHandle.h"

#include <atomic>

bool FSceneHandle::IsValid() const {
    return mId != 0 && mGeneration != 0;
}

bool FSceneHandle::operator==(const FSceneHandle& Other) const {
    return mId == Other.mId && mGeneration == Other.mGeneration;
}

Uint64 AllocateRenderSceneId() {
    static std::atomic<Uint64> NextId{1};

    return NextId.fetch_add(1, std::memory_order_relaxed);
}
