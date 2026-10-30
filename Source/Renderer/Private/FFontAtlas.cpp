/**
 * @file FFontAtlas.cpp
 * @brief MSDF 폰트 메트릭을 읽어 UTF-8 텍스트를 글리프 정점으로 변환합니다.
 * @ingroup Renderer
 */

#include "FFontAtlas.h"

#include <rapidjson/document.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>

/**
 * @brief 폰트 메트릭과 아틀라스 파일을 읽는 내부 보조 함수입니다.
 */
namespace {
    /**
     * @brief JSON 필드가 유한한 수인지 검사하여 실수로 읽습니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     * @param[in] Name 조회하거나 설정할 이름입니다.
     * @param[out] Number 읽은 유한 실수를 기록합니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool ReadNumber(const rapidjson::Value& Value, const char* Name, float& Number) {
        if (!Value.IsObject() || !Value.HasMember(Name) || !Value[Name].IsNumber()) {
            return false;
        }

        Number = static_cast<float>(Value[Name].GetDouble());

        return std::isfinite(Number);
    }

    /**
     * @brief JSON 객체에서 네 성분의 글리프 경계를 읽습니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     * @param[in] Bounds 표시하거나 읽을 경계 상자입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool ReadBounds(const rapidjson::Value& Value, FVector4& Bounds) {
        return ReadNumber(Value, "left", Bounds.x) && ReadNumber(Value, "bottom", Bounds.y) && ReadNumber(Value, "right", Bounds.z) && ReadNumber(Value, "top", Bounds.w) && Bounds.x <= Bounds.z;
    }
}

bool FFontAtlas::Load(const std::filesystem::path& Path) {
    std::error_code Error{};
    const auto Size{std::filesystem::file_size(Path, Error)};

    if (Error || Size == 0 || Size > 16 * 1024 * 1024) {
        return false;
    }

    std::ifstream File{Path, std::ios::binary};
    const FString Text{std::istreambuf_iterator<char>{File}, std::istreambuf_iterator<char>{}};
    rapidjson::Document Document{};
    Document.Parse(Text.data(), Text.size());

    if (Document.HasParseError() || !Document.IsObject() || !Document.HasMember("atlas") || !Document.HasMember("metrics") || !Document.HasMember("glyphs") || !Document["glyphs"].IsArray()) {
        return false;
    }

    const auto& Atlas{Document["atlas"]};
    float Width{};
    float Height{};
    float Range{};
    float LineHeight{};

    if (!ReadNumber(Atlas, "width", Width) || !ReadNumber(Atlas, "height", Height) || !ReadNumber(Atlas, "distanceRange", Range) || !ReadNumber(Document["metrics"], "lineHeight", LineHeight) || Width < 1.0f || Height < 1.0f || Width > 16384.0f || Height > 16384.0f || Width != std::floor(Width) || Height != std::floor(Height) || Range <= 0.0f || Range > 128.0f || LineHeight <= 0.0f || LineHeight > 32.0f) {
        return false;
    }

    if (!Atlas.HasMember("type") || !Atlas["type"].IsString() || (std::string_view{Atlas["type"].GetString()} != "msdf" && std::string_view{Atlas["type"].GetString()} != "mtsdf")) {
        return false;
    }

    bool BottomOrigin{true};

    if (Atlas.HasMember("yOrigin")) {
        if (!Atlas["yOrigin"].IsString()) {
            return false;
        }

        const std::string_view Origin{Atlas["yOrigin"].GetString()};

        if (Origin != "bottom" && Origin != "top") {
            return false;
        }

        BottomOrigin = Origin == "bottom";
    }

    TMap<Uint32, FFontGlyph> Glyphs{};

    for (const auto& Value : Document["glyphs"].GetArray()) {
        FFontGlyph Glyph{};

        if (!Value.IsObject() || !Value.HasMember("unicode") || !Value["unicode"].IsUint() || !ReadNumber(Value, "advance", Glyph.mAdvance) || std::abs(Glyph.mAdvance) > 32.0f) {
            return false;
        }

        if (Value.HasMember("planeBounds") || Value.HasMember("atlasBounds")) {
            FVector4 Pixels{};

            if (!Value.HasMember("planeBounds") || !Value.HasMember("atlasBounds") || !ReadBounds(Value["planeBounds"], Glyph.mPlane) || !ReadBounds(Value["atlasBounds"], Pixels) || Pixels.x < 0.0f || std::min(Pixels.y, Pixels.w) < 0.0f || Pixels.z > Width || std::max(Pixels.y, Pixels.w) > Height || (BottomOrigin ? Pixels.y > Pixels.w : Pixels.w > Pixels.y)) {
                return false;
            }

            if (!BottomOrigin) {
                Glyph.mPlane.y = -Glyph.mPlane.y;
                Glyph.mPlane.w = -Glyph.mPlane.w;
            }

            if (Glyph.mPlane.y > Glyph.mPlane.w) {
                return false;
            }

            const auto Plane{Glyph.mPlane.ToArray()};

            if (!std::all_of(Plane.begin(), Plane.end(), [](float Number) {
                return std::abs(Number) <= 32.0f;
            })) {
                return false;
            }

            Glyph.mUV = {Pixels.x / Width, BottomOrigin ? 1.0f - Pixels.y / Height : Pixels.y / Height, Pixels.z / Width, BottomOrigin ? 1.0f - Pixels.w / Height : Pixels.w / Height};
            Glyph.mHasBounds = true;
        }

        const Uint32 CodePoint{Value["unicode"].GetUint()};

        if (CodePoint > 0x10FFFF || (CodePoint >= 0xD800 && CodePoint <= 0xDFFF) || !Glyphs.emplace(CodePoint, Glyph).second) {
            return false;
        }
    }

    TMap<Uint64, float> Kerning{};

    if (Document.HasMember("kerning")) {
        if (!Document["kerning"].IsArray()) {
            return false;
        }

        for (const auto& Value : Document["kerning"].GetArray()) {
            float Advance{};

            if (!Value.IsObject() || !Value.HasMember("unicode1") || !Value.HasMember("unicode2") || !Value["unicode1"].IsUint() || !Value["unicode2"].IsUint() || !ReadNumber(Value, "advance", Advance) || std::abs(Advance) > 32.0f) {
                return false;
            }

            Kerning.emplace((static_cast<Uint64>(Value["unicode1"].GetUint()) << 32) | Value["unicode2"].GetUint(), Advance);
        }
    }

    if (Glyphs.empty()) {
        return false;
    }

    mGlyphs = std::move(Glyphs);
    mKerning = std::move(Kerning);
    mWidth = static_cast<Uint32>(Width);
    mHeight = static_cast<Uint32>(Height);
    mDistanceRange = Range;
    mLineHeight = LineHeight;

    return true;
}

TArray<Uint32> FFontAtlas::Decode(const FString& Text) {
    TArray<Uint32> Result{};

    for (std::size_t Index{}; Index < Text.size();) {
        const Uint8 First{static_cast<Uint8>(Text[Index])};
        const std::size_t Length{First < 0x80 ? 1u : First >= 0xC2 && First <= 0xDF ? 2u : First >= 0xE0 && First <= 0xEF ? 3u : First >= 0xF0 && First <= 0xF4 ? 4u : 0u};

        if (Length == 0 || Index + Length > Text.size()) {
            Result.push_back(0xFFFD);
            ++Index;
            continue;
        }

        Uint32 CodePoint{static_cast<Uint32>(First & (Length == 1 ? 0x7F : (0x7F >> Length)))};
        bool Valid{true};

        for (std::size_t Offset{1}; Offset < Length; ++Offset) {
            const Uint8 Byte{static_cast<Uint8>(Text[Index + Offset])};
            Valid = Valid && (Byte & 0xC0) == 0x80;
            CodePoint = (CodePoint << 6) | (Byte & 0x3F);
        }

        const Uint32 Minimum{Length == 1 ? 0u : Length == 2 ? 0x80u : Length == 3 ? 0x800u : 0x10000u};
        Valid = Valid && CodePoint >= Minimum && CodePoint <= 0x10FFFF && (CodePoint < 0xD800 || CodePoint > 0xDFFF);
        Result.push_back(Valid ? CodePoint : 0xFFFD);
        Index += Valid ? Length : 1;
    }

    return Result;
}

const FFontGlyph* FFontAtlas::FindGlyph(Uint32 CodePoint) const {
    for (Uint32 Candidate : {CodePoint, 0xFFFDu, static_cast<Uint32>('?')}) {
        const auto Position{mGlyphs.find(Candidate)};

        if (Position != mGlyphs.end()) {
            return &Position->second;
        }
    }

    return nullptr;
}

float FFontAtlas::GetKerning(Uint32 First, Uint32 Second) const {
    const auto Position{mKerning.find((static_cast<Uint64>(First) << 32) | Second)};
    return Position != mKerning.end() ? Position->second : 0.0f;
}

TArray<FTextVertex> FFontAtlas::BuildVertices(const FString& Text) const {
    const TArray<Uint32> CodePoints{Decode(Text)};
    TArray<FTextVertex> Vertices{};
    Vertices.reserve(CodePoints.size() * 6);

    float Baseline{};

    for (std::size_t Start{}; Start < CodePoints.size();) {
        std::size_t End{Start};

        while (End < CodePoints.size() && CodePoints[End] != '\n' && CodePoints[End] != '\r') {
            ++End;
        }

        float Width{};

        for (std::size_t Index{Start}; Index < End; ++Index) {
            const auto* Glyph{FindGlyph(CodePoints[Index] == '\t' ? ' ' : CodePoints[Index])};
            Width += Glyph != nullptr ? Glyph->mAdvance * (CodePoints[Index] == '\t' ? 4.0f : 1.0f) : 0.0f;
            Width += Index + 1 < End ? GetKerning(CodePoints[Index], CodePoints[Index + 1]) : 0.0f;
        }

        float Pen{-Width * 0.5f};

        for (std::size_t Index{Start}; Index < End; ++Index) {
            const auto* Glyph{FindGlyph(CodePoints[Index] == '\t' ? ' ' : CodePoints[Index])};

            if (Glyph == nullptr) {
                continue;
            }

            if (Glyph->mHasBounds && CodePoints[Index] != '\t') {
                const FTextVertex BottomLeft{{Pen + Glyph->mPlane.x, Baseline + Glyph->mPlane.y}, {Glyph->mUV.x, Glyph->mUV.y}};
                const FTextVertex TopLeft{{Pen + Glyph->mPlane.x, Baseline + Glyph->mPlane.w}, {Glyph->mUV.x, Glyph->mUV.w}};
                const FTextVertex TopRight{{Pen + Glyph->mPlane.z, Baseline + Glyph->mPlane.w}, {Glyph->mUV.z, Glyph->mUV.w}};
                const FTextVertex BottomRight{{Pen + Glyph->mPlane.z, Baseline + Glyph->mPlane.y}, {Glyph->mUV.z, Glyph->mUV.y}};
                Vertices.insert(Vertices.end(), {BottomLeft, TopLeft, TopRight, BottomLeft, TopRight, BottomRight});
            }

            Pen += Glyph->mAdvance * (CodePoints[Index] == '\t' ? 4.0f : 1.0f);
            Pen += Index + 1 < End ? GetKerning(CodePoints[Index], CodePoints[Index + 1]) : 0.0f;
        }

        Baseline -= mLineHeight;
        Start = End;

        if (Start < CodePoints.size() && CodePoints[Start++] == '\r' && Start < CodePoints.size() && CodePoints[Start] == '\n') {
            ++Start;
        }
    }

    return Vertices;
}

float FFontAtlas::GetDistanceRange() const {
    return mDistanceRange;
}

Uint32 FFontAtlas::GetWidth() const {
    return mWidth;
}

Uint32 FFontAtlas::GetHeight() const {
    return mHeight;
}
