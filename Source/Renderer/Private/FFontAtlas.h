#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector4.h"

#include <filesystem>

struct FTextVertex {
public:
    ReflectTypes(FTextVertex);

public:
    ReflectStruct(FTextVertex);

private:
    ReflectMembers();

public:
    FVector2 mPosition{};
    ReflectProperty(mPosition, "Position", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector2 mUV{};
    ReflectProperty(mUV, "UV", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

struct FFontGlyph {
public:
    ReflectTypes(FFontGlyph);

public:
    ReflectStruct(FFontGlyph);

private:
    ReflectMembers();

public:
    float mAdvance{};
    ReflectProperty(mAdvance, "Advance", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mPlane{};
    ReflectProperty(mPlane, "Plane", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mUV{};
    ReflectProperty(mUV, "UV", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mHasBounds{};
    ReflectProperty(mHasBounds, "HasBounds", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

class FFontAtlas {
public:
    ReflectTypes(FFontAtlas);

public:
    ReflectStruct(FFontAtlas);

    bool Load(const std::filesystem::path& Path);
    TArray<FTextVertex> BuildVertices(const FString& Text) const;
    float GetDistanceRange() const;
    Uint32 GetWidth() const;
    Uint32 GetHeight() const;

private:
    static TArray<Uint32> Decode(const FString& Text);
    const FFontGlyph* FindGlyph(Uint32 CodePoint) const;
    float GetKerning(Uint32 First, Uint32 Second) const;

private:
    ReflectMembers();

private:
    TMap<Uint32, FFontGlyph> mGlyphs{};
    ReflectProperty(mGlyphs, "Glyphs", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TMap<Uint64, float> mKerning{};
    ReflectProperty(mKerning, "Kerning", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mDistanceRange{4.0f};
    ReflectProperty(mDistanceRange, "DistanceRange", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mLineHeight{1.0f};
    ReflectProperty(mLineHeight, "LineHeight", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mWidth{};
    ReflectProperty(mWidth, "Width", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mHeight{};
    ReflectProperty(mHeight, "Height", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
