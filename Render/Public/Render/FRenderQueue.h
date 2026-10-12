#pragma once
#include "Render/FMeshDrawCommand.h"
#include "Render/FMeshDrawData.h"
#include "RenderCore/FUVTransform.h"

#include <span>

class FRenderScene;

class FRenderQueue {
public:
    void Build(std::span<const std::shared_ptr<const FMeshDrawCommand>> CachedCommands, std::span<const FVisibleMeshDrawCommand> VisibleCommands, bool RenderOpaque, bool RenderTranslucent, const FRenderScene* Scene = nullptr);
    const TArray<FMeshDrawCommandBatch>& GetCommands(ERenderPass Pass) const;
    const TArray<FMeshDrawRecord>& GetDrawRecords() const;
    const TArray<FUVTransform>& GetUVTransforms() const;

private:
    bool IsCommandEnabled(const FMeshDrawCommand& Command, bool RenderOpaque, bool RenderTranslucent) const;

private:
    TArray<FMeshDrawCommandBatch> mOpaqueCommands{};
    TArray<FMeshDrawCommandBatch> mTranslucentCommands{};
    TArray<FMeshDrawCommandBatch> mEmptyCommands{};
    TArray<FMeshDrawRecord> mDrawRecords{};
    TArray<FUVTransform> mUVTransforms{};
    TArray<float> mRecordDepths{};
    TArray<Uint32> mBucketCounts{};
    TArray<Uint32> mBucketWritePositions{};
};
