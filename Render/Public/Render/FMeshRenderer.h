#pragma once
#include "Render/FRenderContext.h"
#include "Render/FRenderQueue.h"

struct FMeshDrawStats {
    Uint64 mPipelineBindCount{};
    Uint64 mTextureBindCount{};
    Uint64 mMeshBindCount{};
    Uint64 mDrawCallCount{};
};

class FMeshRenderer {
public:
    void Draw(const FRenderContext& Context, const TArray<FMeshDrawBatch>& Items, ERenderMode Mode, bool MaterialPass = false);

    const FMeshDrawStats& GetLastDrawStats() const;

private:
    FMeshDrawStats mLastDrawStats{};
};
