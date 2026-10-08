#pragma once
#include "Render/FMeshDrawCommand.h"
#include "Render/FRenderAssetStamp.h"
#include "Core/Base/TCachedValue.h"

#include <map>

struct FMeshDrawCommandKey {
    FAssetHandle mMeshHandle{};
    FAssetHandle mMaterialHandle{};
    FAssetHandle mPipelineHandle{};
    Uint32 mLODLevel{};
    Uint32 mFirstIndex{};
    Uint32 mIndexCount{};
    Uint32 mOriginalIndexCount{};
    Uint32 mMaterialIndex{};
    ERenderPass mPass{ERenderPass::Opaque};
    bool mGPUResources{};
};

bool operator<(const FMeshDrawCommandKey& Left, const FMeshDrawCommandKey& Right);

struct FMeshDrawCommandStamp {
    FRenderAssetStamp mMesh{};
    FRenderAssetStamp mMaterial{};
    FRenderAssetStamp mPipeline{};
    std::array<FRenderAssetStamp, MaxMaterialTextureFields> mTextures{};

    bool operator==(const FMeshDrawCommandStamp&) const = default;
};

class FMeshDrawCommandCache {
private:
    struct FEntry {
        TCachedValue<std::shared_ptr<const FMeshDrawCommand>, FMeshDrawCommandStamp> mCommand{};
        Uint64 mLastUsedUpdate{};
        Uint32 mCommandIndex{};
    };

public:
    void BeginUpdate();
    void EndUpdate();

    template <typename TBuilder>
    Uint32 GetOrCreateCommand(const FMeshDrawCommandKey& Key, const FMeshDrawCommandStamp& Stamp, TBuilder&& Builder);

    const TArray<std::shared_ptr<const FMeshDrawCommand>>& GetCommands() const;

private:
    Uint64 mUpdateSerial{};
    std::map<FMeshDrawCommandKey, FEntry> mEntries{};
    TArray<std::shared_ptr<const FMeshDrawCommand>> mCommands{};
};

template <typename TBuilder>
Uint32 FMeshDrawCommandCache::GetOrCreateCommand(const FMeshDrawCommandKey& Key, const FMeshDrawCommandStamp& Stamp, TBuilder&& Builder) {
    FEntry& Entry{mEntries[Key]};
    const std::shared_ptr<const FMeshDrawCommand>* Command{Entry.mCommand.GetOrUpdate(Stamp, std::forward<TBuilder>(Builder))};

    if (Command == nullptr || *Command == nullptr) {
        Entry.mCommand.Invalidate();
        return UINT32_MAX;
    }

    if (Entry.mLastUsedUpdate != mUpdateSerial) {
        if (mCommands.size() >= UINT32_MAX) {
            return UINT32_MAX;
        }

        Entry.mLastUsedUpdate = mUpdateSerial;
        Entry.mCommandIndex = static_cast<Uint32>(mCommands.size());
        mCommands.push_back(*Command);
    } else {
        mCommands[Entry.mCommandIndex] = *Command;
    }

    return Entry.mCommandIndex;
}
