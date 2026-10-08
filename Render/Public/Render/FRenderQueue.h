#pragma once
#include "Render/FMeshDrawCommand.h"
#include "Render/FMeshDrawData.h"

#include <span>

class FRenderQueue {
public:
    void Build(std::span<const std::shared_ptr<const FMeshDrawCommand>> CachedCommands, std::span<const FVisibleMeshDrawCommand> VisibleCommands, bool RenderOpaque, bool RenderTranslucent);
    const TArray<FMeshDrawCommandBatch>& GetCommands(ERenderPass Pass) const;
    const TArray<FMeshDrawRecord>& GetDrawRecords() const;

private:
    bool IsCommandEnabled(const FMeshDrawCommand& Command, bool RenderOpaque, bool RenderTranslucent) const;

private:
    TArray<FMeshDrawCommandBatch> mOpaqueCommands{};
    TArray<FMeshDrawCommandBatch> mTranslucentCommands{};
    TArray<FMeshDrawCommandBatch> mEmptyCommands{};
    TArray<FMeshDrawRecord> mDrawRecords{};
    TArray<float> mRecordDepths{};
    TArray<Uint32> mBucketCounts{};
    TArray<Uint32> mBucketWritePositions{};
};
