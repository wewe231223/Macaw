#pragma once

#include "Core/Base/FAssetHandle.h"

struct FMaterialTextureMap {
    FString mSourcePath{};
    FAssetHandle mTexture{};
};

struct FMaterialGroup {
    FString mName{};

    FVector3 mAmbient{0.0f, 0.0f, 0.0f};
    FVector3 mDiffuse{1.0f, 1.0f, 1.0f};
    FVector3 mSpecular{0.0f, 0.0f, 0.0f};
    FVector3 mEmissive{0.0f, 0.0f, 0.0f};
    FVector3 mTransmissionFilter{1.0f, 1.0f, 1.0f};

    float mShininess{0.0f};
    float mRefractionIndex{1.0f};
    float mOpacity{1.0f};
    float mSharpness{60.0f};
    Int32 mIlluminationModel{2};
    bool mBDissolveHalo{false};

    FMaterialTextureMap mAmbientTexture{};
    FMaterialTextureMap mDiffuseTexture{};
    FMaterialTextureMap mSpecularTexture{};
    FMaterialTextureMap mEmissiveTexture{};
    FMaterialTextureMap mTransmissionTexture{};
    FMaterialTextureMap mShininessTexture{};
    FMaterialTextureMap mOpacityTexture{};
    FMaterialTextureMap mBumpTexture{};
    FMaterialTextureMap mNormalTexture{};
    FMaterialTextureMap mDisplacementTexture{};
    FMaterialTextureMap mDecalTexture{};
    FMaterialTextureMap mReflectionTexture{};
};
