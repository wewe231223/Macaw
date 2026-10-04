#pragma once
#include "Core/STL.h"
#include "CoreUObject/TypeInfo.h"
#include "Asset/UFont.h"
#include "Math/FMath.h"
#include <ft2build.h>
#include FT_FREETYPE_H

class UFreeTypeFont : public UFont {
public:
    UFreeTypeFont() = default;
    ~UFreeTypeFont() override;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UFreeTypeFont, UFont);

    bool Initialize(const std::filesystem::path& FontPath, Uint32 BakePixelHeight = 32, Uint32 AtlasWidth = 4096, Uint32 AtlasHeight = 4096);
    const FFontGlyph* GetOrCreateGlyph(char32_t CodePoint) override;
    const FFontMetrics& GetFontMetrics() const override;
    std::span<const Uint8> GetAtlasPixels() const override;
    Uint32 GetAtlasWidth() const override;
    Uint32 GetAtlasHeight() const override;
    Uint64 GetAtlasRevision() const override;

    const FFontGlyph* FindGlyph(char32_t CodePoint) const;

    bool AllocateAtlasRect(Uint32 BitmapWidth, Uint32 BitmapHeight, Uint32& OutAtlasX, Uint32& OutAtlasY);
    bool CopyBitmapToAtlas(FT_Bitmap& Bitmap, Uint32 AtlasX, Uint32 AtlasY);

private:
    void Reset();
    bool InitializeFont(const std::filesystem::path& FontPath, Uint32 BakePixelHeight, Uint32 InAtlasWidth, Uint32 InAtlasHeight);

private:
    // FreeType 시스템 핸들
    FT_Library mLibrary{nullptr};
    // 현재 로드된 실제 폰트 파일 하나
    FT_Face mFace{nullptr};
    // Unicode 문자 -> GlyphIndex 매핑
    TMap<char32_t, std::uint32_t> mCodePointToGlyphIndex{};
    // GlyphIndex -> 렌더링 가능한 상태인가?(비트맵으로 저장되어 있는가?)
    TMap<std::uint32_t, FFontGlyph> mGlyphCache{};
    // 현재 Font Face 전체에 공통되는 수직 배치 정보
    FFontMetrics mFontMetrics{};
    // AtlasWidth * AtlasHeight 사이즈의 배열로 2차원 픽셀 좌표의 색값을 1차원 배열로 기록해 해당 픽셀에 글씨가 있는지 없는지 판단
    TArray<std::uint8_t> mAtlasPixels{};
    // 빈 Atlas 텍스처 너비
    std::uint32_t mAtlasWidth{0};
    // 빈 Atlas 텍스처 높이
    std::uint32_t mAtlasHeight{0};
    // 현재 Atlas 행에서 다음 glyph가 배치될 X 시작 위치
    std::uint32_t mNextAtlasX{0};
    // 현재 glyph를 배치하고 있는 Atlas 행의 Y 시작 위치
    std::uint32_t mNextAtlasY{0};
    // 현재 행에서 가장 높은 Bitmap 높이
    std::uint32_t mCurrentRowHeight{0};
    // Atlas에서 Glyph 상하좌우 패딩
    static constexpr std::uint32_t AtlasPadding{1};
    Uint64 mAtlasRevision{1};
    bool mBInitialized{false};
};
