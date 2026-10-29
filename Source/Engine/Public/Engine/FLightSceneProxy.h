#pragma once

#include "Core/Guid.h"
#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector4.h"
#include "Engine/ELightType.h"

class UTexture2D;
struct FPrimitiveSceneProxy;

struct FLightSceneProxy {
public:
    ReflectTypes(FLightSceneProxy);

public:
    ReflectStruct(FLightSceneProxy);

private:
    ReflectMembers();

public:
    FPrimitiveSceneProxy CreateIconProxy() const;

    FGuid mId{};
    ReflectProperty(mId, "Id", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    ELightType mType{ELightType::Ambient};
    ReflectProperty(mType, "Type", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mPosition{};
    ReflectProperty(mPosition, "Position", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector mDirection{1.0f, 0.0f, 0.0f};
    ReflectProperty(mDirection, "Direction", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mIntensity{1.0f};
    ReflectProperty(mIntensity, "Intensity", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mAttenuationRadius{10.0f};
    ReflectProperty(mAttenuationRadius, "AttenuationRadius", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mLightFalloffExponent{2.0f};
    ReflectProperty(mLightFalloffExponent, "LightFalloffExponent", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mInnerConeAngle{20.0f};
    ReflectProperty(mInnerConeAngle, "InnerConeAngle", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mOuterConeAngle{35.0f};
    ReflectProperty(mOuterConeAngle, "OuterConeAngle", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mVisible{true};
    ReflectProperty(mVisible, "Visible", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    UTexture2D* mIcon{};
    ReflectProperty(mIcon, "Icon", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mCastShadows{true};
    ReflectProperty(mCastShadows, "CastShadows", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mShadowResolution{1024};
    ReflectProperty(mShadowResolution, "ShadowResolution", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mShadowResolutionScale{1.0f};
    ReflectProperty(mShadowResolutionScale, "ShadowResolutionScale", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mShadowBias{0.0005f};
    ReflectProperty(mShadowBias, "ShadowBias", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mShadowSlopeBias{1.0f};
    ReflectProperty(mShadowSlopeBias, "ShadowSlopeBias", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mShadowSharpen{};
    ReflectProperty(mShadowSharpen, "ShadowSharpen", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mCascadeCount{4};
    ReflectProperty(mCascadeCount, "CascadeCount", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mShadowDistance{100.0f};
    ReflectProperty(mShadowDistance, "ShadowDistance", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mCascadeTransitionFraction{0.1f};
    ReflectProperty(mCascadeTransitionFraction, "CascadeTransitionFraction", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
