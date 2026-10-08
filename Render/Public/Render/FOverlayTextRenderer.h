#pragma once

#include "Render/FFrameResource.h"
#include "Render/FRenderAssetResources.h"
#include "RenderCore/FOverlayRenderData.h"

class IAssetRegistryMutator;

class FOverlayTextRenderer {
private:
    struct FCachedText {
        FAssetHandle mFontHandle{};
        FGuid mFontGuid{};
        float mPixelHeight{};
        float mLetterSpacing{};
        float mLineSpacing{};
        Uint64 mFontRevision{};
        Uint64 mLastUsedFrame{};
        TArray<FTextVertex> mVertices{};
    };

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
    void BindAssetRegistry(const IAssetRegistry* Registry, IAssetRegistryMutator* Mutator);
    void BeginFrame(Uint64 FrameSerial);
    void Reset();
    void Render(ID3D11DeviceContext* Context, FFrameResource& FrameResource, const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport, const TArray<FOverlayTextDrawData>& Draws, FRenderAssetResources& Resources);

private:
    const TArray<FTextVertex>* GetTextGeometry(const FOverlayTextDrawData& Data, const UFont& Font, FAssetHandle FontHandle);
    bool ProjectAnchor(const FOverlayTextDrawData& Data, const TArray<FTextVertex>& Vertices, const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport, FVector2& Position) const;

private:
    ID3D11Device* mDevice{nullptr};
    const IAssetRegistry* mAssetRegistry{nullptr};
    IAssetRegistryMutator* mAssetRegistryMutator{nullptr};
    Uint64 mFrameSerial{};
    TMap<FString, TArray<FCachedText>> mTextCache{};
    FPipelineRenderResource mPipeline{};
    Microsoft::WRL::ComPtr<ID3D11SamplerState> mSampler{};
    TArray<FGlyphInstance> mGlyphs{};
    TArray<FTextDraw> mDraws{};
};
