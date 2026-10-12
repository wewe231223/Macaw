#pragma once
#include "Core/CoreMinimal.h"
#include "Core/Base/FAssetHandle.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "RenderCore/FRenderData.generated.h"

USTRUCT()
struct FTextVertex {
public:
    GENERATED_TYPES(FTextVertex)

public:
    GENERATED_BODY(FTextVertex)

public:
    UPROPERTY(Serialize, Name = "LocalPosition")
    FVector2 mLocalPosition{};
    UPROPERTY(Serialize, Name = "Size")
    FVector2 mSize{};
    UPROPERTY(Serialize, Name = "UvMin")
    FVector2 mUvMin{};
    UPROPERTY(Serialize, Name = "UvMax")
    FVector2 mUvMax{};
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

USTRUCT()
struct FPostProcessingSettings {
public:
    GENERATED_TYPES(FPostProcessingSettings)

public:
    GENERATED_BODY(FPostProcessingSettings)

public:
    UPROPERTY(Serialize, Name = "FXAA")
    bool mFXAA{true};
};

USTRUCT()
struct FRenderSettings {
public:
    GENERATED_TYPES(FRenderSettings)

public:
    GENERATED_BODY(FRenderSettings)

public:
    UPROPERTY(Serialize, Name = "ClearColor")
    FVector4 mClearColor{0.2f, 0.2f, 0.7f, 1.0f};
    UPROPERTY(Serialize, Name = "BRenderSky")
    bool mBRenderSky{true};
    UPROPERTY(Serialize, Name = "PostProcessing")
    FPostProcessingSettings mPostProcessing{};
};

UENUM()
enum class ELightType : Uint32 {
    Directional,
    Point,
    Spot
};

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
