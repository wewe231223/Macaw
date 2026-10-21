#include "pch.h"
#include "Render/FRenderQueue.h"
#include "Render/FRenderScene.h"

#include <algorithm>
#include <cmath>

void FRenderQueue::Build(std::span<const std::shared_ptr<const FMeshDrawCommand>> CachedCommands, std::span<const FVisibleMeshDrawCommand> VisibleCommands, bool RenderOpaque, bool RenderTranslucent, const FRenderScene* Scene) {
    mOpaqueCommands.clear();
    mTranslucentCommands.clear();
    mDrawRecords.clear();
    mUVTransforms.clear();
    mUVTransforms.push_back(FUVTransform{});
    mRecordDepths.clear();

    if ((!RenderOpaque && !RenderTranslucent) || CachedCommands.empty() || VisibleCommands.empty()) {
        return;
    }

    mBucketCounts.assign(CachedCommands.size(), 0);
    mBucketWritePositions.resize(CachedCommands.size());

    for (const FVisibleMeshDrawCommand& Visible : VisibleCommands) {
        if (Visible.mCommandIndex >= CachedCommands.size() || CachedCommands[Visible.mCommandIndex] == nullptr || !IsCommandEnabled(*CachedCommands[Visible.mCommandIndex], RenderOpaque, RenderTranslucent)) {
            continue;
        }

        Uint32& Count{mBucketCounts[Visible.mCommandIndex]};

        if (Count == UINT32_MAX) {
            return;
        }

        ++Count;
    }

    std::size_t TotalRecords{};

    for (std::size_t Bucket{}; Bucket < mBucketCounts.size(); ++Bucket) {
        const Uint32 Count{mBucketCounts[Bucket]};

        if (TotalRecords > UINT32_MAX - static_cast<std::size_t>(Count)) {
            return;
        }

        mBucketWritePositions[Bucket] = static_cast<Uint32>(TotalRecords);
        TotalRecords += Count;
    }

    mDrawRecords.resize(TotalRecords);
    mRecordDepths.resize(TotalRecords);

    for (const FVisibleMeshDrawCommand& Visible : VisibleCommands) {
        if (Visible.mCommandIndex >= CachedCommands.size() || CachedCommands[Visible.mCommandIndex] == nullptr || !IsCommandEnabled(*CachedCommands[Visible.mCommandIndex], RenderOpaque, RenderTranslucent)) {
            continue;
        }

        const FMeshDrawCommand& Command{*CachedCommands[Visible.mCommandIndex]};
        const Uint32 Destination{mBucketWritePositions[Visible.mCommandIndex]++};

        Uint32 UVIndex{};

        if (Scene != nullptr && Visible.mObjectIndex < Scene->GetPrimitives().size()) {
            const FPrimitiveSceneInfo& Primitive{Scene->GetPrimitives()[Visible.mObjectIndex]};

            if (Primitive.mActive && Primitive.mProxy != nullptr) {
                const FUVTransform Transform{Primitive.mProxy->GetUVTransform(Command.mMaterialSlotName)};

                if (Transform.mScale.mX != 1.0f || Transform.mScale.mY != 1.0f || Transform.mOffset.mX != 0.0f || Transform.mOffset.mY != 0.0f) {
                    UVIndex = static_cast<Uint32>(mUVTransforms.size());
                    mUVTransforms.push_back(Transform);
                }
            }
        }

        mDrawRecords[Destination] = FMeshDrawRecord{Visible.mObjectIndex, Command.mMaterialIndex, Visible.mLODDither, UVIndex};
        mRecordDepths[Destination] = std::isfinite(Visible.mSortDepth) ? Visible.mSortDepth : 0.0f;
    }

    for (std::size_t Bucket{}; Bucket < mBucketCounts.size(); ++Bucket) {
        const Uint32 Count{mBucketCounts[Bucket]};

        if (Count == 0) {
            continue;
        }

        const std::shared_ptr<const FMeshDrawCommand>& Command{CachedCommands[Bucket]};
        const Uint32 FirstRecord{mBucketWritePositions[Bucket] - Count};

        if (Command->mPass == ERenderPass::Opaque) {
            mOpaqueCommands.push_back(FMeshDrawCommandBatch{Command, FirstRecord, Count});
            continue;
        }

        for (Uint32 Index{}; Index < Count; ++Index) {
            const Uint32 RecordIndex{FirstRecord + Index};

            mTranslucentCommands.push_back(FMeshDrawCommandBatch{Command, RecordIndex, 1, mRecordDepths[RecordIndex]});
        }
    }

    std::stable_sort(mTranslucentCommands.begin(), mTranslucentCommands.end(), [](const FMeshDrawCommandBatch& Left, const FMeshDrawCommandBatch& Right) {
        return Left.mSortDepth > Right.mSortDepth;
    });
}

const TArray<FMeshDrawCommandBatch>& FRenderQueue::GetCommands(ERenderPass Pass) const {
    switch (Pass) {
        case ERenderPass::Opaque:
            return mOpaqueCommands;

        case ERenderPass::Translucent:
            return mTranslucentCommands;

        default:
            return mEmptyCommands;
    }
}

const TArray<FMeshDrawRecord>& FRenderQueue::GetDrawRecords() const {
    return mDrawRecords;
}

const TArray<FUVTransform>& FRenderQueue::GetUVTransforms() const {
    return mUVTransforms;
}

bool FRenderQueue::IsCommandEnabled(const FMeshDrawCommand& Command, bool RenderOpaque, bool RenderTranslucent) const {
    return (Command.mPass == ERenderPass::Opaque && RenderOpaque) || (Command.mPass == ERenderPass::Translucent && RenderTranslucent);
}
