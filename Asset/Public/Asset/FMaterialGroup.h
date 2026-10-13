#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "Core/Base/FName.h"
#include "Core/Base/FAssetHandle.h"
#include "Asset/UTexture.h"

struct FMaterialTextureMap {
public:
    ReflectTypes(FMaterialTextureMap);

public:
    ReflectStruct(FMaterialTextureMap);

    void SetTexture(FAssetHandle Texture);

public:
    FString mSourcePath{};
    ReflectSetter(mTexture, SetTexture, "Texture", EPropertyFlags::Edit | EPropertyFlags::Asset, ReflectAsset(UTexture));
    FAssetHandle mTexture{};

    ReflectMembers();
};

struct FMaterialGroup {
public:
    ReflectTypes(FMaterialGroup);

public:
    ReflectStruct(FMaterialGroup);

public:
    FName mName{};
    ReflectProperty(mAmbient, "Ambient", EPropertyFlags::Edit | EPropertyFlags::Color, ReflectEditor("Material", "Ambient"));
    FVector3 mAmbient{0.0f, 0.0f, 0.0f};
    ReflectProperty(mDiffuse, "Diffuse", EPropertyFlags::Edit | EPropertyFlags::Color, ReflectEditor("Material", "Diffuse"));
    FVector4 mDiffuse{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectProperty(mSpecular, "Specular", EPropertyFlags::Edit | EPropertyFlags::Color, ReflectEditor("Material", "Specular"));
    FVector3 mSpecular{0.0f, 0.0f, 0.0f};
    ReflectProperty(mEmissive, "Emissive", EPropertyFlags::Edit | EPropertyFlags::Color, ReflectEditor("Material", "Emissive"));
    FVector3 mEmissive{0.0f, 0.0f, 0.0f};
    ReflectProperty(mTransmissionFilter, "TransmissionFilter", EPropertyFlags::Edit, ReflectEditor("Material", "Transmission Filter"), ReflectRange(0.0f, 1.0f), ReflectSpeed(0.01f));
    FVector3 mTransmissionFilter{1.0f, 1.0f, 1.0f};
    ReflectProperty(mShininess, "Shininess", EPropertyFlags::Edit, ReflectEditor("Material", "Shininess"), ReflectRange(0.0f, 1000.0f), ReflectSpeed(1.0f));
    float mShininess{0.0f};
    ReflectProperty(mRefractionIndex, "RefractionIndex", EPropertyFlags::Edit, ReflectEditor("Material", "Refraction Index"), ReflectRange(0.0f, 10.0f), ReflectSpeed(0.01f));
    float mRefractionIndex{1.0f};
    ReflectProperty(mSharpness, "Sharpness", EPropertyFlags::Edit, ReflectEditor("Material", "Sharpness"), ReflectRange(0.0f, 1000.0f), ReflectSpeed(1.0f));
    float mSharpness{60.0f};
    ReflectProperty(mIlluminationModel, "IlluminationModel", EPropertyFlags::Edit, ReflectEditor("Material", "Illumination Model"), ReflectSpeed(1.0f));
    Int32 mIlluminationModel{2};
    ReflectProperty(mBDissolveHalo, "BDissolveHalo", EPropertyFlags::Edit, ReflectEditor("Material", "Dissolve Halo"));
    bool mBDissolveHalo{false};
    ReflectProperty(mAmbientTexture, "AmbientTexture", EPropertyFlags::Edit, ReflectEditor("Textures", "Ambient"));
    FMaterialTextureMap mAmbientTexture{};
    ReflectProperty(mDiffuseTexture, "DiffuseTexture", EPropertyFlags::Edit, ReflectEditor("Textures", "Diffuse"));
    FMaterialTextureMap mDiffuseTexture{};
    ReflectProperty(mSpecularTexture, "SpecularTexture", EPropertyFlags::Edit, ReflectEditor("Textures", "Specular"));
    FMaterialTextureMap mSpecularTexture{};
    ReflectProperty(mEmissiveTexture, "EmissiveTexture", EPropertyFlags::Edit, ReflectEditor("Textures", "Emissive"));
    FMaterialTextureMap mEmissiveTexture{};
    ReflectProperty(mTransmissionTexture, "TransmissionTexture", EPropertyFlags::Edit, ReflectEditor("Textures", "Transmission"));
    FMaterialTextureMap mTransmissionTexture{};
    ReflectProperty(mShininessTexture, "ShininessTexture", EPropertyFlags::Edit, ReflectEditor("Textures", "Shininess"));
    FMaterialTextureMap mShininessTexture{};
    ReflectProperty(mOpacityTexture, "OpacityTexture", EPropertyFlags::Edit, ReflectEditor("Textures", "Opacity"));
    FMaterialTextureMap mOpacityTexture{};
    ReflectProperty(mBumpTexture, "BumpTexture", EPropertyFlags::Edit, ReflectEditor("Textures", "Bump"));
    FMaterialTextureMap mBumpTexture{};
    ReflectProperty(mNormalTexture, "NormalTexture", EPropertyFlags::Edit, ReflectEditor("Textures", "Normal"));
    FMaterialTextureMap mNormalTexture{};
    ReflectProperty(mDisplacementTexture, "DisplacementTexture", EPropertyFlags::Edit, ReflectEditor("Textures", "Displacement"));
    FMaterialTextureMap mDisplacementTexture{};
    ReflectProperty(mDecalTexture, "DecalTexture", EPropertyFlags::Edit, ReflectEditor("Textures", "Decal"));
    FMaterialTextureMap mDecalTexture{};
    ReflectProperty(mReflectionTexture, "ReflectionTexture", EPropertyFlags::Edit, ReflectEditor("Textures", "Reflection"));
    FMaterialTextureMap mReflectionTexture{};

    ReflectMembers();
};
