#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "Render/FMeshDrawCommandCache.h"
#include "Render/FRenderAssetResources.h"
#include "Asset/UMaterial.h"
#include "Asset/UMesh.h"
#include "Asset/UTexture.h"

#include <tuple>

namespace {
    bool BuildShaderBindings(const UMesh& Mesh, const UPipeline& Pipeline, FRenderAssetResources& Resources, FMeshDrawCommand& Command) {
        const IAssetRegistry* Registry{FAssetRegistry::TryGet()};

        if (Registry == nullptr) {
            return false;
        }

        const FMeshRenderResource* MeshResource{Resources.GetMesh(Mesh)};
        const FPipelineRenderResource* PipelineResource{Resources.GetPipeline(Pipeline)};

        if (MeshResource == nullptr || PipelineResource == nullptr) {
            return false;
        }

        for (std::size_t Index{}; Index < Command.mPipelineStates.size(); ++Index) {
            const ERenderMode RequestedMode{static_cast<ERenderMode>(Index)};
            const ERenderMode ResolvedMode{Pipeline.ResolveRenderMode(RequestedMode)};

            if ((RequestedMode != ERenderMode::Outline || Pipeline.RenderModeSettable(RequestedMode)) && Pipeline.RenderModeSettable(ResolvedMode)) {
                PipelineResource->BuildMaterialState(ResolvedMode, Command.mState.mBlendMode, Command.mPipelineStates[Index]);
            }
        }

        constexpr std::array<EVertexAttribute, 4> Attributes{EVertexAttribute::Position, EVertexAttribute::Normal, EVertexAttribute::UV, EVertexAttribute::Color};

        for (std::size_t Index{}; Index < Attributes.size(); ++Index) {
            Command.mVertexBuffers[Index] = MeshResource->GetVertexBuffer(Attributes[Index], Command.mState.mLODLevel);
            Command.mVertexStrides[Index] = Mesh.GetVertexStride(Attributes[Index]);
        }

        for (Uint8 Index{}; Index < Command.mState.mTextureSignature.mTextureFieldCount; ++Index) {
            const UTexture* Texture{Registry->ResolveAsset<UTexture>(Command.mState.mTextureSignature.GetTextureHandle(Index))};

            Command.mTextures[Index] = Texture != nullptr ? Resources.GetTexture(*Texture) : nullptr;

            if (Texture != nullptr && Command.mTextures[Index] == nullptr) {
                return false;
            }
        }

        return true;
    }
}

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

void FMeshDrawCommandCache::AddMeshBatch(const FMeshBatch& Mesh, const FMaterialBuffer& Materials, FRenderAssetResources* Resources, TArray<Uint32>& OutCommands) {
    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};

    if (Registry == nullptr) {
        return;
    }

    const UMesh* MeshAsset{Registry->ResolveAsset<UMesh>(Mesh.mMeshHandle)};
    const UMaterial* Material{Registry->ResolveAsset<UMaterial>(Mesh.mMaterialHandle)};
    const UPipeline* Pipeline{Registry->ResolveAsset<UPipeline>(Mesh.mPipelineHandle)};

    if (MeshAsset == nullptr || Material == nullptr || Pipeline == nullptr) {
        return;
    }

    for (const FMeshBatchElement& Element : Mesh.mElements) {
        FMeshDrawState State{};
        Uint32 MaterialIndex{UINT32_MAX};

        if (!BuildMeshDrawState(Mesh, Element, *Material, Materials, State, MaterialIndex) || (State.mBlendMode != EMaterialBlendMode::Opaque && State.mBlendMode != EMaterialBlendMode::Translucent)) {
            continue;
        }

        const ERenderPass Pass{State.mBlendMode == EMaterialBlendMode::Opaque ? ERenderPass::Opaque : ERenderPass::Translucent};
        const FMeshDrawCommandKey Key{Mesh.mMeshHandle, Mesh.mMaterialHandle, Mesh.mPipelineHandle, State.mLODLevel, State.mFirstIndex, State.mIndexCount, State.mOriginalIndexCount, MaterialIndex, Pass, Resources != nullptr};
        FMeshDrawCommandStamp Stamp{FRenderAssetStamp{MeshAsset->GetHandle(), MeshAsset->GetRenderRevision()}, FRenderAssetStamp{Material->GetHandle(), Material->GetRenderRevision()}, FRenderAssetStamp{Pipeline->GetHandle(), Pipeline->GetRenderRevision()}};

        for (Uint8 Index{}; Index < State.mTextureSignature.mTextureFieldCount; ++Index) {
            const FAssetHandle Handle{State.mTextureSignature.GetTextureHandle(Index)};
            const UTexture* Texture{Registry->ResolveAsset<UTexture>(Handle)};

            Stamp.mTextures[Index] = Texture != nullptr ? FRenderAssetStamp{Texture->GetHandle(), Texture->GetRenderRevision()} : FRenderAssetStamp{};
        }

        const Uint32 CommandIndex{GetOrCreateCommand(Key, Stamp, [&](std::shared_ptr<const FMeshDrawCommand>& Command) {
            std::shared_ptr<FMeshDrawCommand> NewCommand{std::make_shared<FMeshDrawCommand>()};

            NewCommand->mState = State;
            NewCommand->mMaterialIndex = MaterialIndex;
            NewCommand->mMaterialGroupName = Material->GetGroupName(State.mMaterialGroupIndex);
            NewCommand->mPass = Pass;

            if (Resources != nullptr && !BuildShaderBindings(*MeshAsset, *Pipeline, *Resources, *NewCommand)) {
                return false;
            }

            Command = std::move(NewCommand);

            return true;
        })};

        if (CommandIndex != UINT32_MAX) {
            OutCommands.push_back(CommandIndex);
        }
    }
}

const TArray<std::shared_ptr<const FMeshDrawCommand>>& FMeshDrawCommandCache::GetCommands() const {
    return mCommands;
}
