#include "pch.h"
#include "RenderCore/FRenderProbe.h"

#include <atomic>

Uint64 AllocateRenderSceneId() {
    static std::atomic<Uint64> NextId{1};

    return NextId.fetch_add(1, std::memory_order_relaxed);
}
