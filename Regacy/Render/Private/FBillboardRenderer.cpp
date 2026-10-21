#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "Render/FBillboardRenderer.h"
#include "Asset/Pipeline/UPipeline.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/UTexture.h"
#include "Render/FRenderBuffers.h"
#include <algorithm>
#include <unordered_map>

bool FBillboardRenderer::FBatchKey::operator==(const FBatchKey& Other) const {
    return mPipelineHandle == Other.mPipelineHandle && mTextureHandle == Other.mTextureHandle;
}

std::size_t FBillboardRenderer::FBatchKeyHash::operator()(const FBatchKey& Key) const noexcept {
    return std::hash<std::uint32_t>{}(Key.mPipelineHandle.mId) ^ (std::hash<std::uint32_t>{}(Key.mTextureHandle.mId) << 1);
}

bool FBillboardRenderer::Initialize(ID3D11Device* InDevice, std::uint32_t InitialCapacity) {
    if (InDevice == nullptr || InitialCapacity == 0) {
        return false;
    }

    mDevice = InDevice;

    return true;
}

void FBillboardRenderer::Render(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const TArray<FBillboardDrawData>& BillboardDraws, FRenderAssetResources& Resources, ERenderMode Mode) {
    const IAssetRegistry* AssetRegistry{FAssetRegistry::TryGet()};

    if (Context == nullptr || mDevice == nullptr || AssetRegistry == nullptr || BillboardDraws.empty() || !RenderBuffers.HasCameraWorld() || !RenderBuffers.BindCommon(Context)) {
        return;
    }

    mInstances.clear();
    mDraws.clear();

    if (BillboardDraws.size() > UINT32_MAX / sizeof(FBillboardData)) {
        return;
    }

    std::unordered_map<FBatchKey, FBillboardBatch, FBatchKeyHash> Batches{};

    for (const FBillboardDrawData& Data : BillboardDraws) {
        if (!Data.mPipelineHandle || !Data.mTextureHandle) {
            continue;
        }

        ++Batches[FBatchKey{Data.mPipelineHandle, Data.mTextureHandle}].mDraw.mInstanceCount;
    }

    Uint32 InstanceCount{};

    for (auto& [Key, Batch] : Batches) {
        const UPipeline* Pipeline{AssetRegistry->ResolveAsset<UPipeline>(Key.mPipelineHandle)};
        const UTexture* Texture{AssetRegistry->ResolveAsset<UTexture>(Key.mTextureHandle)};

        if (Pipeline == nullptr || Texture == nullptr) {
            continue;
        }

        Batch.mDraw.mPipeline = Pipeline;
        Batch.mDraw.mTexture = Texture;
        Batch.mDraw.mFirstInstance = InstanceCount;
        InstanceCount += Batch.mDraw.mInstanceCount;
        mDraws.push_back(Batch.mDraw);
    }

    mInstances.resize(InstanceCount);

    for (const FBillboardDrawData& Data : BillboardDraws) {
        const auto Iterator{Batches.find(FBatchKey{Data.mPipelineHandle, Data.mTextureHandle})};

        if (Iterator == Batches.end() || Iterator->second.mDraw.mPipeline == nullptr) {
            continue;
        }

        FBillboardBatch& Batch{Iterator->second};

        mInstances[Batch.mDraw.mFirstInstance + Batch.mWriteCount] = FBillboardData{Data.mWorld, Data.mSize, Data.mUvMin, Data.mUvMax, FVector2{}, Data.mColor};
        ++Batch.mWriteCount;
    }

    if (mDraws.empty() || !RenderBuffers.UploadStream(mDevice, Context, ERenderStream::Billboard, mInstances.data(), static_cast<Uint32>(mInstances.size()), sizeof(FBillboardData), D3D11_BIND_VERTEX_BUFFER)) {
        return;
    }

    ID3D11Buffer* Buffer{RenderBuffers.GetStreamBuffer(ERenderStream::Billboard)};
    const UINT Stride{sizeof(FBillboardData)};
    const UINT Offset{};

    Context->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
    Context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

    for (const FBillboardDraw& Draw : mDraws) {
        const FPipelineRenderResource* Pipeline{Resources.GetPipeline(*Draw.mPipeline)};
        ID3D11ShaderResourceView* TextureSRV{Resources.GetTexture(*Draw.mTexture)};

        if (Pipeline == nullptr || TextureSRV == nullptr) {
            continue;
        }

        Pipeline->Bind(Context, Draw.mPipeline->ResolveRenderMode(Mode));
        Context->PSSetShaderResources(3, 1, &TextureSRV);
        Context->DrawInstanced(1, Draw.mInstanceCount, 0, Draw.mFirstInstance);
    }
}
