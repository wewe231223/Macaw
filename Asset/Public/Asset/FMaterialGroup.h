#pragma once
#include "Core/Base/FName.h"
#include "Core/Base/FAssetHandle.h"
#include "Asset/UTexture.h"
#include "Asset/FMaterialGroup.generated.h"

USTRUCT()
struct FMaterialTextureMap {
public:
    GENERATED_TYPES(FMaterialTextureMap)

public:
    GENERATED_BODY(FMaterialTextureMap)

    void SetTexture(FAssetHandle Texture);

public:
    FString mSourcePath{};
    UPROPERTY(Edit, Asset, AssetClass = UTexture, Setter = SetTexture, DisplayName = "Texture")
    FAssetHandle mTexture{};
};

USTRUCT()
struct FMaterialGroup {
public:
    GENERATED_TYPES(FMaterialGroup)

public:
    GENERATED_BODY(FMaterialGroup)

public:
    FName mName{};

    UPROPERTY(Edit, Category = "Material", Color)
    FVector3 mAmbient{0.0f, 0.0f, 0.0f};
    UPROPERTY(Edit, Category = "Material", Color)
    FVector4 mDiffuse{1.0f, 1.0f, 1.0f, 1.0f};
    UPROPERTY(Edit, Category = "Material", Color)
    FVector3 mSpecular{0.0f, 0.0f, 0.0f};
    UPROPERTY(Edit, Category = "Material", Color)
    FVector3 mEmissive{0.0f, 0.0f, 0.0f};
    UPROPERTY(Edit, Category = "Material", Min = 0.0f, Max = 1.0f, DisplayName = "Transmission Filter", Speed = 0.01f)
    FVector3 mTransmissionFilter{1.0f, 1.0f, 1.0f};

    UPROPERTY(Edit, Category = "Material", Min = 0.0f, Max = 1000.0f, Speed = 1.0f)
    float mShininess{0.0f};
    UPROPERTY(Edit, Category = "Material", Min = 0.0f, Max = 10.0f, DisplayName = "Refraction Index", Speed = 0.01f)
    float mRefractionIndex{1.0f};
    UPROPERTY(Edit, Category = "Material", Min = 0.0f, Max = 1000.0f, Speed = 1.0f)
    float mSharpness{60.0f};
    UPROPERTY(Edit, Category = "Material", DisplayName = "Illumination Model", Speed = 1.0f)
    Int32 mIlluminationModel{2};
    UPROPERTY(Edit, Category = "Material", DisplayName = "Dissolve Halo")
    bool mBDissolveHalo{false};

    UPROPERTY(Edit, Category = "Textures", DisplayName = "Ambient")
    FMaterialTextureMap mAmbientTexture{};
    UPROPERTY(Edit, Category = "Textures", DisplayName = "Diffuse")
    FMaterialTextureMap mDiffuseTexture{};
    UPROPERTY(Edit, Category = "Textures", DisplayName = "Specular")
    FMaterialTextureMap mSpecularTexture{};
    UPROPERTY(Edit, Category = "Textures", DisplayName = "Emissive")
    FMaterialTextureMap mEmissiveTexture{};
    UPROPERTY(Edit, Category = "Textures", DisplayName = "Transmission")
    FMaterialTextureMap mTransmissionTexture{};
    UPROPERTY(Edit, Category = "Textures", DisplayName = "Shininess")
    FMaterialTextureMap mShininessTexture{};
    UPROPERTY(Edit, Category = "Textures", DisplayName = "Opacity")
    FMaterialTextureMap mOpacityTexture{};
    UPROPERTY(Edit, Category = "Textures", DisplayName = "Bump")
    FMaterialTextureMap mBumpTexture{};
    UPROPERTY(Edit, Category = "Textures", DisplayName = "Normal")
    FMaterialTextureMap mNormalTexture{};
    UPROPERTY(Edit, Category = "Textures", DisplayName = "Displacement")
    FMaterialTextureMap mDisplacementTexture{};
    UPROPERTY(Edit, Category = "Textures", DisplayName = "Decal")
    FMaterialTextureMap mDecalTexture{};
    UPROPERTY(Edit, Category = "Textures", DisplayName = "Reflection")
    FMaterialTextureMap mReflectionTexture{};
};
