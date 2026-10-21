/**
 * @file FTextRenderer.h
 * @brief 전역 에셋 조회를 사용하는 월드 텍스트 렌더러를 정의합니다.
 */
#pragma once

#include <d3d11.h>
#include "Math/FMath.h"
#include "Core/STL.h"
#include "RenderCore/FRenderData.h"
#include "Render/FRenderAssetResources.h"

class FRenderBuffers;
class UPipeline;

class FTextRenderer {
private:
    struct FTextContext {
        FMatrix mWorld{};
        FVector4 mColor{};
    };

    struct FGlyphVertex {
        FTextVertex mGlyph{};
        Uint32 mTextIndex{};
    };

    struct FTextDraw {
        const UPipeline* mPipeline{};
        ID3D11ShaderResourceView* mAtlas{};
        Uint32 mFirstVertex{};
        Uint32 mVertexCount{};
    };

    static_assert(sizeof(FTextContext) == 80);
    static_assert(sizeof(FGlyphVertex) == 36);

public:
    bool Initialize(ID3D11Device* InDevice, std::uint32_t InitialCapacity = 256);
    /**
     * @brief 전역 레지스트리의 폰트와 파이프라인으로 월드 텍스트를 렌더링합니다.
     * @param Context 그래픽 명령 컨텍스트입니다.
     * @param RenderBuffers 이 렌더러의 버퍼 저장소입니다.
     * @param TextDraws 렌더링할 월드별 텍스트 목록입니다.
     * @param Resources 장치에 귀속된 GPU 에셋 캐시입니다.
     * @note 에셋 서비스가 없으면 아무것도 그리지 않습니다.
     */
    void Render(ID3D11DeviceContext* Context, FRenderBuffers& RenderBuffers, const TArray<FTextDrawData>& TextDraws, FRenderAssetResources& Resources);

private:
    ID3D11Device* mDevice{nullptr};
    TArray<FTextContext> mTextContexts{};
    TArray<FGlyphVertex> mVertices{};
    TArray<FTextDraw> mDraws{};
};
