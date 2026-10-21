#pragma once
#include "Render/FRenderContext.h"
#include "Render/FMeshDrawCommand.h"
#include "Asset/Pipeline/UPipeline.h"

struct FMeshDrawStats {
    Uint64 mPipelineBindCount{};
    Uint64 mTextureBindCount{};
    Uint64 mMeshBindCount{};
    Uint64 mDrawCallCount{};
};

class FMeshRenderer {
public:
    void Draw(const FRenderContext& Context, const TArray<FMeshDrawCommandBatch>& Commands, ERenderMode Mode);

    const FMeshDrawStats& GetLastDrawStats() const;

private:
    FMeshDrawStats mLastDrawStats{};
};
