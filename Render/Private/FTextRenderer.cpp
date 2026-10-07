#include "pch.h"
#include "Render/FTextRenderer.h"
#include "Asset/Pipeline/UPipeline.h"
#include "Render/FRenderAssetResources.h"
#include "Asset/UFont.h"
#include "Render/FFrameResource.h"

#include <algorithm>
#include <limits>

bool FTextRenderer::Initialize(ID3D11Device* InDevice, std::uint32_t InitialCapacity) {
    if (InDevice == nullptr || InitialCapacity == 0) {
        return false;
    }

    mDevice = InDevice;

    return true;
}

void FTextRenderer::Render(ID3D11DeviceContext* Context, FFrameResource& FrameResource, const TArray<FTextProbe>& TextProbes, const IAssetRegistry* AssetRegistry, FRenderAssetResources& Resources) {
    if (Context == nullptr || mDevice == nullptr || AssetRegistry == nullptr || TextProbes.empty() || !FrameResource.HasCameraWorld() || !FrameResource.BindCommon(Context)) {
        return;
    }

    mTextContexts.clear();
    mVertices.clear();
    mDraws.clear();

    constexpr std::size_t MaxVertexCount{UINT32_MAX / sizeof(FGlyphVertex)};
    constexpr std::size_t MaxTextCount{UINT32_MAX / sizeof(FTextContext)};

    for (const FTextProbe& Probe : TextProbes) {
        if (Probe.mVertices.empty()) {
            continue;
        }

        const UFont* Font{AssetRegistry->ResolveAsset<UFont>(Probe.mFontHandle)};

        if (Font == nullptr) {
            continue;
        }

        ID3D11ShaderResourceView* AtlasSRV{Resources.GetFontAtlas(*Font, Context)};

        if (AtlasSRV == nullptr) {
            continue;
        }

        const UPipeline* PipeLine{AssetRegistry->ResolveAsset<UPipeline>(Probe.mPipelineHandle)};

        if (PipeLine == nullptr) {
            continue;
        }

        if (Probe.mVertices.size() > MaxVertexCount - mVertices.size() || mTextContexts.size() >= MaxTextCount) {
            return;
        }

        const Uint32 TextIndex{static_cast<Uint32>(mTextContexts.size())};

        mTextContexts.push_back(FTextContext{Probe.mWorld, Probe.mColor});
        mDraws.push_back(FTextDraw{PipeLine, AtlasSRV, static_cast<Uint32>(mVertices.size()), static_cast<Uint32>(Probe.mVertices.size())});

        for (const FTextVertex& Vertex : Probe.mVertices) {
            mVertices.push_back(FGlyphVertex{Vertex, TextIndex});
        }
    }

    if (mDraws.empty()) {
        return;
    }

    ID3D11ShaderResourceView* NullResource{};

    Context->GSSetShaderResources(0, 1, &NullResource);

    if (!FrameResource.UploadStream(mDevice, Context, EFrameStream::Text, mVertices.data(), static_cast<Uint32>(mVertices.size()), sizeof(FGlyphVertex), D3D11_BIND_VERTEX_BUFFER) || !FrameResource.UploadStream(mDevice, Context, EFrameStream::TextContext, mTextContexts.data(), static_cast<Uint32>(mTextContexts.size()), sizeof(FTextContext), D3D11_BIND_SHADER_RESOURCE)) {
        return;
    }

    ID3D11Buffer* Buffer{FrameResource.GetStreamBuffer(EFrameStream::Text)};
    ID3D11ShaderResourceView* TextContexts{FrameResource.GetStreamResourceView(EFrameStream::TextContext)};
    const UINT Stride{sizeof(FGlyphVertex)};
    const UINT Offset{};

    Context->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
    Context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    Context->GSSetShaderResources(0, 1, &TextContexts);

    for (const FTextDraw& Draw : mDraws) {
        const FPipelineRenderResource* Pipeline{Resources.GetPipeline(*Draw.mPipeline)};

        if (Pipeline == nullptr) {
            continue;
        }

        Pipeline->Bind(Context, Draw.mPipeline->GetRenderMode());
        Context->PSSetShaderResources(3, 1, &Draw.mAtlas);
        Context->DrawInstanced(Draw.mVertexCount, 1, Draw.mFirstVertex, 0);
    }
}
