#pragma once

#include <d3d11.h>
#include "Math/FMath.h"
#include "Core/STL.h"
#include "RenderCore/FRenderProbe.h"
#include "Render/FRenderAssetResources.h"

class FFrameResource;
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
    void Render(ID3D11DeviceContext* Context, FFrameResource& FrameResource, const TArray<FTextProbe>& TextProbes, const IAssetRegistry* AssetRegistry, FRenderAssetResources& Resources);

private:
    ID3D11Device* mDevice{nullptr};
    TArray<FTextContext> mTextContexts{};
    TArray<FGlyphVertex> mVertices{};
    TArray<FTextDraw> mDraws{};
};
