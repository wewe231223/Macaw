#include "pch.h"
#include "Render/FRenderAssetResources.h"
#include "Render/FMeshRenderer.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/UMesh.h"
#include "Asset/UTexture.h"
#include "Render/FFrameResource.h"
#include "Core/Stat/Stat.h"

void FMeshRenderer::Draw(const FRenderContext& Context, const TArray<FMeshDrawBatch>& Items, ERenderMode Mode) {
    mLastDrawStats = {};

    if (Items.empty() || Context.mDeviceContext == nullptr || Context.mAssetRegistry == nullptr || Context.mAssetResources == nullptr || Context.mFrameResource == nullptr) {
        return;
    }

    ID3D11DeviceContext* DeviceContext{Context.mDeviceContext};

    if (!Context.mFrameResource->BindModels(DeviceContext)) {
        return;
    }

    DeviceContext->VSSetShaderResources(1, 1, &Context.mMaterialResource);
    DeviceContext->PSSetShaderResources(1, 1, &Context.mMaterialResource);

    const UPipeline* BoundPipeline{};
    ERenderMode BoundMode{ERenderMode::Lit};
    Uint32 BoundStencilReference{};
    const FMaterialChunkSignature* BoundTextures{};
    const UMesh* BoundMesh{};
    Uint32 BoundLOD{};
    EMaterialBlendMode BoundBlendMode{EMaterialBlendMode::Opaque};

    for (std::size_t BatchIndex{}; BatchIndex < Items.size(); ++BatchIndex) {
        const FMeshDrawBatch& Item{Items[BatchIndex]};
        const FMeshDrawState& State{Item.mState};
        const UPipeline* Pipeline{Context.mAssetRegistry->ResolveAsset<UPipeline>(State.mPipelineHandle)};
        const UMesh* Mesh{Context.mAssetRegistry->ResolveAsset<UMesh>(State.mMeshHandle)};

        if (Pipeline == nullptr || Mesh == nullptr || Item.mRecordCount == 0 || (Mode == ERenderMode::Outline && !Pipeline->RenderModeSettable(Mode))) {
            continue;
        }

        const FPipelineRenderResource* PipelineResource{Context.mAssetResources->GetPipeline(*Pipeline)};
        const FMeshRenderResource* MeshResource{Context.mAssetResources->GetMesh(*Mesh)};

        if (PipelineResource == nullptr || MeshResource == nullptr) {
            continue;
        }

        const ERenderMode ResolvedMode{Pipeline->ResolveRenderMode(Mode)};

        if (!Pipeline->RenderModeSettable(ResolvedMode)) {
            continue;
        }

        const Uint32 StencilReference{ResolvedMode == ERenderMode::Outline ? 1u : 0u};

        if (BoundPipeline != Pipeline || BoundMode != ResolvedMode || BoundStencilReference != StencilReference || BoundBlendMode != State.mBlendMode) {
            PipelineResource->BindMaterial(DeviceContext, ResolvedMode, State.mBlendMode, StencilReference);

            BoundPipeline = Pipeline;
            BoundMode = ResolvedMode;
            BoundStencilReference = StencilReference;
            BoundBlendMode = State.mBlendMode;
            ++mLastDrawStats.mPipelineBindCount;
        }

        if (BoundTextures == nullptr || *BoundTextures != State.mTextureSignature) {
            std::array<ID3D11ShaderResourceView*, MaxMaterialTextureFields> TextureResources{};

            for (Uint8 Index{}; Index < State.mTextureSignature.mTextureFieldCount; ++Index) {
                const UTexture* Texture{Context.mAssetRegistry->ResolveAsset<UTexture>(State.mTextureSignature.GetTextureHandle(Index))};

                TextureResources[Index] = Texture != nullptr ? Context.mAssetResources->GetTexture(*Texture) : nullptr;
            }

            DeviceContext->VSSetShaderResources(3, static_cast<UINT>(TextureResources.size()), TextureResources.data());
            DeviceContext->PSSetShaderResources(3, static_cast<UINT>(TextureResources.size()), TextureResources.data());
            BoundTextures = &State.mTextureSignature;
            ++mLastDrawStats.mTextureBindCount;
        }

        const int LODLevel{static_cast<int>(State.mLODLevel)};

        if (BoundMesh != Mesh || BoundLOD != State.mLODLevel) {
            ID3D11Buffer* VertexBuffers[]{MeshResource->GetVertexBuffer(EVertexAttribute::Position, LODLevel), MeshResource->GetVertexBuffer(EVertexAttribute::Normal, LODLevel), MeshResource->GetVertexBuffer(EVertexAttribute::UV, LODLevel), MeshResource->GetVertexBuffer(EVertexAttribute::Color, LODLevel)};
            const Uint32 Strides[]{Mesh->GetVertexStride(EVertexAttribute::Position), Mesh->GetVertexStride(EVertexAttribute::Normal), Mesh->GetVertexStride(EVertexAttribute::UV), Mesh->GetVertexStride(EVertexAttribute::Color)};
            const Uint32 Offsets[]{0, 0, 0, 0};

            DeviceContext->IASetVertexBuffers(0, _countof(VertexBuffers), VertexBuffers, Strides, Offsets);
            BoundMesh = Mesh;
            BoundLOD = State.mLODLevel;
            ++mLastDrawStats.mMeshBindCount;
        }

        DeviceContext->DrawInstanced(State.mIndexCount, Item.mRecordCount, State.mFirstIndex, Item.mFirstRecord);
        ++mLastDrawStats.mDrawCallCount;

        Stat::RecordLODStats(State.mLODLevel, static_cast<std::uint64_t>(State.mIndexCount / 3) * Item.mRecordCount, static_cast<std::uint64_t>(State.mOriginalIndexCount / 3) * Item.mRecordCount, 1);
    }
}

const FMeshDrawStats& FMeshRenderer::GetLastDrawStats() const {
    return mLastDrawStats;
}
