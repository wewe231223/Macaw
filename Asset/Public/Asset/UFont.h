#pragma once
#include "Asset/UAsset.h"
#include "Core/Base/FAssetHandle.h"
#include "Math/FVector.h"
#include <array>
#include <cstdint>
#include <span>

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

public:
    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(UFont, UAsset)

    virtual const FFontGlyph* GetOrCreateGlyph(char32_t CodePoint) = 0;
    virtual const FFontMetrics& GetFontMetrics() const = 0;
    virtual std::span<const Uint8> GetAtlasPixels() const = 0;
    virtual Uint32 GetAtlasWidth() const = 0;
    virtual Uint32 GetAtlasHeight() const = 0;
    virtual Uint64 GetAtlasRevision() const = 0;
};
