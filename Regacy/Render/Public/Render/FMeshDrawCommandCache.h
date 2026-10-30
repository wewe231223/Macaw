/**
 * @file FMeshDrawCommandCache.h
 * @brief 전역 에셋을 조회하며 씬별 메시 명령을 유지하는 캐시를 정의합니다.
 */

#pragma once
#include "Render/FMeshDrawCommand.h"
#include "Render/FRenderAssetStamp.h"
#include "Core/Base/TCachedValue.h"

#include <map>

class FRenderAssetResources;

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

    /**
     * @brief 전역 에셋으로 메시 배치의 렌더 명령을 생성하거나 갱신합니다.
     * @param Mesh 처리할 메시 배치입니다.
     * @param Materials 장치별 머티리얼 저장소입니다.
     * @param Resources 선택적인 GPU 에셋 캐시입니다.
     * @param OutCommands 생성하거나 재사용한 명령 인덱스를 추가할 배열입니다.
     */
    void AddMeshBatch(const FMeshBatch& Mesh, const FMaterialBuffer& Materials, FRenderAssetResources* Resources, TArray<Uint32>& OutCommands);

    const TArray<std::shared_ptr<const FMeshDrawCommand>>& GetCommands() const;

private:
    template <typename TBuilder>
    Uint32 GetOrCreateCommand(const FMeshDrawCommandKey& Key, const FMeshDrawCommandStamp& Stamp, TBuilder&& Builder);

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
