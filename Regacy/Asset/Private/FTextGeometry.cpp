#include "pch.h"
#include "Asset/FTextGeometry.h"
#include "Asset/IAssetRegistryMutator.h"

#include <algorithm>
#include <climits>
#include <cmath>

bool BuildTextGeometry(const UFont& Font, IAssetRegistryMutator& Registry, FAssetHandle FontHandle, const FString& Text, float CharacterHeight, float LetterSpacing, float LineSpacing, TArray<FTextVertex>& Vertices) {
    Vertices.clear();

    const FFontMetrics& Metrics{Font.GetFontMetrics()};

    if (Text.empty()) {
        return true;
    }

    if (!FontHandle || Text.size() > INT_MAX || !std::isfinite(CharacterHeight) || CharacterHeight <= 0.0f || !std::isfinite(LetterSpacing) || !std::isfinite(LineSpacing) || !std::isfinite(Metrics.mLineHeight) || Metrics.mLineHeight <= 0.0f) {
        return false;
    }

    const int WideLength{MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Text.data(), static_cast<int>(Text.size()), nullptr, 0)};

    if (WideLength <= 0) {
        return false;
    }

    std::wstring WideText{};

    WideText.resize(WideLength);

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Text.data(), static_cast<int>(Text.size()), WideText.data(), WideLength) != WideLength) {
        return false;
    }

    const float Scale{CharacterHeight / Metrics.mLineHeight};
    float PenX{};
    float BaselineY{};

    for (std::size_t Index{}; Index < WideText.size(); ++Index) {
        char32_t CodePoint{static_cast<char32_t>(WideText[Index])};

        if (CodePoint >= 0xD800 && CodePoint <= 0xDBFF && Index + 1 < WideText.size()) {
            const char32_t Low{static_cast<char32_t>(WideText[++Index])};

            CodePoint = 0x10000 + ((CodePoint - 0xD800) << 10) + Low - 0xDC00;
        }

        if (CodePoint == U'\r') {
            continue;
        }

        if (CodePoint == U'\n') {
            PenX = 0.0f;
            BaselineY -= CharacterHeight + LineSpacing;
            continue;
        }

        const FFontGlyph* Glyph{Registry.GetOrCreateFontGlyph(FontHandle, CodePoint)};

        if (Glyph == nullptr) {
            Glyph = Registry.GetOrCreateFontGlyph(FontHandle, U'\uFFFD');
        }

        if (Glyph == nullptr) {
            Glyph = Registry.GetOrCreateFontGlyph(FontHandle, U'?');
        }

        if (Glyph == nullptr) {
            continue;
        }

        if (Glyph->mBitmapWidth > 0 && Glyph->mBitmapHeight > 0) {
            FTextVertex Vertex{};

            Vertex.mLocalPosition = FVector2{PenX + static_cast<float>(Glyph->mBearingX) * Scale, BaselineY + static_cast<float>(Glyph->mBearingY) * Scale};
            Vertex.mSize = FVector2{static_cast<float>(Glyph->mBitmapWidth) * Scale, static_cast<float>(Glyph->mBitmapHeight) * Scale};
            Vertex.mUvMin = Glyph->mUvMin;
            Vertex.mUvMax = Glyph->mUvMax;
            Vertices.push_back(Vertex);
        }

        PenX += Glyph->mAdvanceX * Scale + LetterSpacing;
    }

    if (Vertices.empty()) {
        return true;
    }

    const FTextVertex& First{Vertices.front()};
    float Left{First.mLocalPosition.mX};
    float Right{Left + First.mSize.mX};
    float Top{First.mLocalPosition.mY};
    float Bottom{Top - First.mSize.mY};

    for (const FTextVertex& Vertex : Vertices) {
        Left = std::min(Left, Vertex.mLocalPosition.mX);
        Right = std::max(Right, Vertex.mLocalPosition.mX + Vertex.mSize.mX);
        Top = std::max(Top, Vertex.mLocalPosition.mY);
        Bottom = std::min(Bottom, Vertex.mLocalPosition.mY - Vertex.mSize.mY);
    }

    const FVector2 Center{(Left + Right) * 0.5f, (Top + Bottom) * 0.5f};

    for (FTextVertex& Vertex : Vertices) {
        Vertex.mLocalPosition.mX -= Center.mX;
        Vertex.mLocalPosition.mY -= Center.mY;
    }

    return true;
}
