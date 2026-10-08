#include "pch.h"
#include "Render/FMeshPassProcessor.h"
#include "Render/FRenderAssetResources.h"
#include "Asset/UMaterial.h"
#include "Asset/UMesh.h"
#include "Asset/UTexture.h"

FMeshPassProcessor::FMeshPassProcessor(ERenderPass Pass, const IAssetRegistry& Registry, const FMaterialBuffer& Materials, FRenderAssetResources* Resources, FMeshDrawCommandCache& Cache)
	: mPass(Pass),
	  mRegistry(Registry),
	  mMaterials(Materials),
	  mResources(Resources),
	  mCache(Cache) {
}

FMeshPassProcessor::~FMeshPassProcessor() = default;

void FMeshPassProcessor::AddMeshBatch(const FMeshBatch& Mesh, TArray<Uint32>& OutCommands) {
    const UMesh* MeshAsset{mRegistry.ResolveAsset<UMesh>(Mesh.mMeshHandle)};
    const UMaterial* Material{mRegistry.ResolveAsset<UMaterial>(Mesh.mMaterialHandle)};
    const UPipeline* Pipeline{mRegistry.ResolveAsset<UPipeline>(Mesh.mPipelineHandle)};

    if (MeshAsset == nullptr || Material == nullptr || Pipeline == nullptr) {
        return;
    }

    for (const FMeshBatchElement& Element : Mesh.mElements) {
        FMeshDrawState State{};
        Uint32 MaterialIndex{UINT32_MAX};

        if (!BuildMeshDrawState(Mesh, Element, mRegistry, mMaterials, State, MaterialIndex) || !AcceptsMaterial(State.mBlendMode)) {
            continue;
        }

        const FMeshDrawCommandKey Key{Mesh.mMeshHandle, Mesh.mMaterialHandle, Mesh.mPipelineHandle, State.mLODLevel, State.mFirstIndex, State.mIndexCount, State.mOriginalIndexCount, MaterialIndex, mPass, mResources != nullptr};
        FMeshDrawCommandStamp Stamp{FRenderAssetStamp{MeshAsset->GetHandle(), MeshAsset->GetRenderRevision()}, FRenderAssetStamp{Material->GetHandle(), Material->GetRenderRevision()}, FRenderAssetStamp{Pipeline->GetHandle(), Pipeline->GetRenderRevision()}};

        for (Uint8 Index{}; Index < State.mTextureSignature.mTextureFieldCount; ++Index) {
            const FAssetHandle Handle{State.mTextureSignature.GetTextureHandle(Index)};
            const UTexture* Texture{mRegistry.ResolveAsset<UTexture>(Handle)};

            Stamp.mTextures[Index] = Texture != nullptr ? FRenderAssetStamp{Texture->GetHandle(), Texture->GetRenderRevision()} : FRenderAssetStamp{};
        }

        const Uint32 CommandIndex{mCache.GetOrCreateCommand(Key, Stamp, [&](std::shared_ptr<const FMeshDrawCommand>& Command) {
            std::shared_ptr<FMeshDrawCommand> NewCommand{std::make_shared<FMeshDrawCommand>()};

            NewCommand->mState = State;
            NewCommand->mMaterialIndex = MaterialIndex;
            NewCommand->mPass = mPass;

            if (!BuildShaderBindings(Mesh, *NewCommand)) {
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

bool FMeshPassProcessor::AcceptsMaterial(EMaterialBlendMode BlendMode) const {
    return (mPass == ERenderPass::Opaque && BlendMode == EMaterialBlendMode::Opaque) || (mPass == ERenderPass::Translucent && BlendMode == EMaterialBlendMode::Translucent);
}

bool FMeshPassProcessor::BuildShaderBindings(const FMeshBatch& Mesh, FMeshDrawCommand& Command) {
    if (mResources == nullptr) {
        return true;
    }

    const UMesh* MeshAsset{mRegistry.ResolveAsset<UMesh>(Mesh.mMeshHandle)};
    const UPipeline* Pipeline{mRegistry.ResolveAsset<UPipeline>(Mesh.mPipelineHandle)};
    const FMeshRenderResource* MeshResource{mResources->GetMesh(*MeshAsset)};
    const FPipelineRenderResource* PipelineResource{mResources->GetPipeline(*Pipeline)};

    if (MeshResource == nullptr || PipelineResource == nullptr) {
        return false;
    }

    for (std::size_t Index{}; Index < Command.mPipelineStates.size(); ++Index) {
        const ERenderMode RequestedMode{static_cast<ERenderMode>(Index)};
        const ERenderMode ResolvedMode{Pipeline->ResolveRenderMode(RequestedMode)};

        if ((RequestedMode != ERenderMode::Outline || Pipeline->RenderModeSettable(RequestedMode)) && Pipeline->RenderModeSettable(ResolvedMode)) {
            PipelineResource->BuildMaterialState(ResolvedMode, Command.mState.mBlendMode, Command.mPipelineStates[Index]);
        }
    }

    constexpr std::array<EVertexAttribute, 4> Attributes{EVertexAttribute::Position, EVertexAttribute::Normal, EVertexAttribute::UV, EVertexAttribute::Color};

    for (std::size_t Index{}; Index < Attributes.size(); ++Index) {
        Command.mVertexBuffers[Index] = MeshResource->GetVertexBuffer(Attributes[Index], Mesh.mLODLevel);
        Command.mVertexStrides[Index] = MeshAsset->GetVertexStride(Attributes[Index]);
    }

    for (Uint8 Index{}; Index < Command.mState.mTextureSignature.mTextureFieldCount; ++Index) {
        const UTexture* Texture{mRegistry.ResolveAsset<UTexture>(Command.mState.mTextureSignature.GetTextureHandle(Index))};

        Command.mTextures[Index] = Texture != nullptr ? mResources->GetTexture(*Texture) : nullptr;

        if (Texture != nullptr && Command.mTextures[Index] == nullptr) {
            return false;
        }
    }

    return true;
}
