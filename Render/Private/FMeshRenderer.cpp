#include "pch.h"
#include "Render/FMeshRenderer.h"
#include "Render/FFrameResource.h"
#include "Core/Stat/Stat.h"

void FMeshRenderer::Draw(const FRenderContext& Context, const TArray<FMeshDrawCommandBatch>& Commands, ERenderMode Mode) {
    mLastDrawStats = {};

    const std::size_t ModeIndex{static_cast<std::size_t>(Mode)};

    if (Commands.empty() || ModeIndex >= static_cast<std::size_t>(ERenderMode::Max) || Context.mDeviceContext == nullptr || Context.mFrameResource == nullptr || !Context.mFrameResource->BindModels(Context.mDeviceContext)) {
        return;
    }

    ID3D11DeviceContext* DeviceContext{Context.mDeviceContext};

    DeviceContext->VSSetShaderResources(1, 1, &Context.mMaterialResource);
    DeviceContext->PSSetShaderResources(1, 1, &Context.mMaterialResource);

    const FMeshDrawPipelineState* BoundPipeline{nullptr};
    std::array<ID3D11ShaderResourceView*, MaxMaterialTextureFields> BoundTextures{};
    std::array<ID3D11Buffer*, 4> BoundVertices{};
    std::array<Uint32, 4> BoundStrides{};
    bool TexturesBound{};
    bool VerticesBound{};

    for (const FMeshDrawCommandBatch& Batch : Commands) {
        if (Batch.mCommand == nullptr || Batch.mRecordCount == 0) {
            continue;
        }

        const FMeshDrawCommand& Command{*Batch.mCommand};
        const FMeshDrawPipelineState& Pipeline{Command.mPipelineStates[ModeIndex]};

        if (!Pipeline.IsValid()) {
            continue;
        }

        if (BoundPipeline == nullptr || !(*BoundPipeline == Pipeline)) {
            Pipeline.Bind(DeviceContext, Mode == ERenderMode::Outline ? 1u : 0u);
            BoundPipeline = &Pipeline;
            ++mLastDrawStats.mPipelineBindCount;
        }

        std::array<ID3D11ShaderResourceView*, MaxMaterialTextureFields> Textures{};

        for (std::size_t Index{}; Index < Textures.size(); ++Index) {
            Textures[Index] = Command.mTextures[Index].Get();
        }

        if (!TexturesBound || BoundTextures != Textures) {
            DeviceContext->VSSetShaderResources(3, static_cast<UINT>(Textures.size()), Textures.data());
            DeviceContext->PSSetShaderResources(3, static_cast<UINT>(Textures.size()), Textures.data());
            BoundTextures = Textures;
            TexturesBound = true;
            ++mLastDrawStats.mTextureBindCount;
        }

        std::array<ID3D11Buffer*, 4> Vertices{};

        for (std::size_t Index{}; Index < Vertices.size(); ++Index) {
            Vertices[Index] = Command.mVertexBuffers[Index].Get();
        }

        if (!VerticesBound || BoundVertices != Vertices || BoundStrides != Command.mVertexStrides) {
            constexpr std::array<Uint32, 4> Offsets{};

            DeviceContext->IASetVertexBuffers(0, static_cast<UINT>(Vertices.size()), Vertices.data(), Command.mVertexStrides.data(), Offsets.data());
            BoundVertices = Vertices;
            BoundStrides = Command.mVertexStrides;
            VerticesBound = true;
            ++mLastDrawStats.mMeshBindCount;
        }

        DeviceContext->DrawInstanced(Command.mState.mIndexCount, Batch.mRecordCount, Command.mState.mFirstIndex, Batch.mFirstRecord);
        ++mLastDrawStats.mDrawCallCount;
        Stat::RecordLODStats(Command.mState.mLODLevel, static_cast<Uint64>(Command.mState.mIndexCount / 3) * Batch.mRecordCount, static_cast<Uint64>(Command.mState.mOriginalIndexCount / 3) * Batch.mRecordCount, 1);
    }
}

const FMeshDrawStats& FMeshRenderer::GetLastDrawStats() const {
    return mLastDrawStats;
}
