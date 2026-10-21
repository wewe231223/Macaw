/**
 * @file FOverlayTextRenderer.h
 * @brief 전역 폰트 서비스를 사용하는 오버레이 텍스트 렌더러를 정의합니다.
 */
#pragma once

#include "Render/FRenderBuffers.h"
#include "Render/FRenderAssetResources.h"
#include "RenderCore/FOverlayRenderData.h"

class FOverlayTextRenderer {
private:
    struct FCachedText {
        FAssetHandle mFontHandle{};
        /**
         * @brief 텍스트가 참조하는 폰트의 객체 식별자입니다.
         */
        FGuid mFontGuid{};
        /** @brief 텍스트 지오메트리를 생성한 전역 에셋 서비스 세대입니다. */
        Uint64 mRegistryGeneration{};
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
    /** @brief 현재 전역 에셋 서비스에서 폰트의 생존과 세대를 확인하고 오래된 텍스트 캐시를 제거합니다. @param FrameSerial 현재 프레임 번호입니다. */
    void BeginFrame(Uint64 FrameSerial);
    void Reset();
    void Render(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport, const TArray<FOverlayTextDrawData>& Draws, FRenderAssetResources& Resources);

private:
    const TArray<FTextVertex>* GetTextGeometry(const FOverlayTextDrawData& Data, const UFont& Font, FAssetHandle FontHandle);
    bool ProjectAnchor(const FOverlayTextDrawData& Data, const TArray<FTextVertex>& Vertices, const FViewMatrices& Camera, const D3D11_VIEWPORT& Viewport, FVector2& Position) const;

private:
    ID3D11Device* mDevice{nullptr};
    Uint64 mFrameSerial{};
    TMap<FString, TArray<FCachedText>> mTextCache{};
    FPipelineRenderResource mPipeline{};
    Microsoft::WRL::ComPtr<ID3D11SamplerState> mSampler{};
    TArray<FGlyphInstance> mGlyphs{};
    TArray<FTextDraw> mDraws{};
};
