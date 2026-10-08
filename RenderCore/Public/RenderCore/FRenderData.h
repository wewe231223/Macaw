#pragma once
#include "Core/CoreMinimal.h"
#include "Core/Base/FAssetHandle.h"

struct FTextVertex {
    FVector2 mLocalPosition{};
    FVector2 mSize{};
    FVector2 mUvMin{};
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

struct FPostProcessingSettings {
    bool mFXAA{true};
};

struct FRenderSettings {
    FVector4 mClearColor{0.2f, 0.2f, 0.7f, 1.0f};
    bool mBRenderSky{true};
    FPostProcessingSettings mPostProcessing{};
};

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
