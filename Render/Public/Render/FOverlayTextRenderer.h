#pragma once

#include "Render/FFrameResource.h"
#include "Render/FRenderAssetResources.h"
#include "RenderCore/FOverlayRenderData.h"

class FOverlayTextRenderer {
private:
    struct FGlyphInstance {
        FTextVertex mGlyph{};
        FVector4 mColor{};
    };

    struct FTextDraw {
        ID3D11ShaderResourceView* mAtlas{nullptr};
        Uint32 mFirstGlyph{};
        Uint32 mGlyphCount{};
    };

    static_assert(sizeof(FGlyphInstance) == 48);

public:
    bool Initialize(ID3D11Device* Device);
    void Reset();
    void Render(ID3D11DeviceContext* Context, FFrameResource& FrameResource, const CameraProbe& Camera, const D3D11_VIEWPORT& Viewport, const TArray<FOverlayTextProbe>& Probes, const IAssetRegistry& Registry, FRenderAssetResources& Resources);

private:
    bool ProjectAnchor(const FOverlayTextProbe& Probe, const CameraProbe& Camera, const D3D11_VIEWPORT& Viewport, FVector2& Position) const;

private:
    ID3D11Device* mDevice{nullptr};
    FPipelineRenderResource mPipeline{};
    Microsoft::WRL::ComPtr<ID3D11SamplerState> mSampler{};
    TArray<FGlyphInstance> mGlyphs{};
    TArray<FTextDraw> mDraws{};
};
