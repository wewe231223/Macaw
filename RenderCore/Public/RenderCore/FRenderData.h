#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "Core/CoreMinimal.h"
#include "Core/Base/FAssetHandle.h"

struct FTextVertex {
public:
    ReflectTypes(FTextVertex);

public:
    ReflectStruct(FTextVertex);

public:
    ReflectProperty(mLocalPosition, "LocalPosition", EPropertyFlags::Serialize, ReflectSerializer(mLocalPosition));
    FVector2 mLocalPosition{};
    ReflectProperty(mSize, "Size", EPropertyFlags::Serialize, ReflectSerializer(mSize));
    FVector2 mSize{};
    ReflectProperty(mUvMin, "UvMin", EPropertyFlags::Serialize, ReflectSerializer(mUvMin));
    FVector2 mUvMin{};
    ReflectProperty(mUvMax, "UvMax", EPropertyFlags::Serialize, ReflectSerializer(mUvMax));
    FVector2 mUvMax{};

    ReflectMembers();
};

struct FTextDrawData {
    FMatrix mWorld{};
    FAssetHandle mFontHandle{};
    FAssetHandle mPipelineHandle{};
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
    TArray<FTextVertex> mVertices{};
};

struct FBillboardDrawData {
    FMatrix mWorld{};
    FAssetHandle mTextureHandle{};
    FAssetHandle mPipelineHandle{};
    FVector2 mSize{};
    FVector2 mUvMin{};
    FVector2 mUvMax{};
    FVector4 mColor{};
};

struct FViewMatrices {
    FMatrix mViewProjection{};
    FMatrix mView{};
    FMatrix mProjection{};
    FFrustum mViewFrustum{};
};

struct FPostProcessingSettings {
public:
    ReflectTypes(FPostProcessingSettings);

public:
    ReflectStruct(FPostProcessingSettings);

public:
    ReflectProperty(mFXAA, "FXAA", EPropertyFlags::Serialize, ReflectSerializer(mFXAA));
    bool mFXAA{true};

    ReflectMembers();
};

struct FRenderSettings {
public:
    ReflectTypes(FRenderSettings);

public:
    ReflectStruct(FRenderSettings);

public:
    ReflectProperty(mClearColor, "ClearColor", EPropertyFlags::Serialize, ReflectSerializer(mClearColor));
    FVector4 mClearColor{0.2f, 0.2f, 0.7f, 1.0f};
    ReflectProperty(mBRenderSky, "BRenderSky", EPropertyFlags::Serialize, ReflectSerializer(mBRenderSky));
    bool mBRenderSky{true};
    ReflectProperty(mPostProcessing, "PostProcessing", EPropertyFlags::Serialize, ReflectSerializer(mPostProcessing));
    FPostProcessingSettings mPostProcessing{};

    ReflectMembers();
};

enum class ELightType : Uint32 {
    Directional,
    Point,
    Spot
};

ReflectEnum(ELightType, ReflectEnumValue(ELightType, Directional), ReflectEnumValue(ELightType, Point), ReflectEnumValue(ELightType, Spot));

struct FLightShaderParameters {
    FVector3 mColor{1.0f, 1.0f, 1.0f};
    float mIntensity{1.0f};
    FVector3 mPosition{};
    float mAttenuationRadius{};
    FVector3 mDirection{0.0f, 0.0f, 1.0f};
    float mInnerConeCos{1.0f};
    float mOuterConeCos{1.0f};
    ELightType mType{ELightType::Directional};
    FVector2 mPadding{};
};

static_assert(sizeof(FLightShaderParameters) == 64);
