#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Guid.h"
#include "Core/Math/FVector4.h"

struct FShadowAtlasTile {
public:
    ReflectTypes(FShadowAtlasTile);

public:
    ReflectStruct(FShadowAtlasTile);

private:
    ReflectMembers();

public:
    FGuid mLightId{};
    ReflectProperty(mLightId, "LightId", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mRectangle{};
    ReflectProperty(mRectangle, "Rectangle", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mFace{};
    ReflectProperty(mFace, "Face", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mResolution{};
    ReflectProperty(mResolution, "Resolution", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

struct FShadowAtlasView {
public:
    ReflectTypes(FShadowAtlasView);

public:
    ReflectStruct(FShadowAtlasView);

private:
    ReflectMembers();

public:
    void* mTexture{};
    ReflectProperty(mTexture, "Texture", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mSize{};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FShadowAtlasTile> mTiles{};
    ReflectProperty(mTiles, "Tiles", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    void* mDepthTexture{};
    ReflectProperty(mDepthTexture, "DepthTexture", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
