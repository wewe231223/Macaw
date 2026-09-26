#pragma once
#include "UAsset.h"
#include "Core/Base/FAssetHandle.h"
#include "Math/FVector.h"
#include <array>
#include <cstdint>
#include <d3d11.h>

struct FFontGlyph {
    Uint32 mGlyphIndex{0};

    Uint32 mAtlasX{0};
    Uint32 mAtlasY{0};
    Uint32 mBitmapWidth{0};
    Uint32 mBitmapHeight{0};

    Int32 mBearingX{0};
    Int32 mBearingY{0};
    float mAdvanceX{0.0f};
    float mAdvanceY{0.0f};

    FVector2 mUvMin{};
    FVector2 mUvMax{};
};

struct FFontMetrics {
    float mBakePixelHeight{0.0f};
    float mAscender{0.0f};
    float mDescender{0.0f};
    float mLineHeight{0.0f};
};

class UFont : public UAsset {
public:
    UFont() = default;
    ~UFont() override = default;

    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(UFont, UAsset)

    virtual const FFontGlyph* GetOrCreateGlyph(char32_t CodePoint) = 0;
    virtual const FFontMetrics& GetFontMetrics() const = 0;
    virtual void FlushAtlas(ID3D11DeviceContext* Context) = 0;
    virtual ID3D11ShaderResourceView* GetAtlasSRV() const = 0;
};
