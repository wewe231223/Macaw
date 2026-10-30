#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "Core/Base/FName.h"
#include "Core/Base/FAssetHandle.h"
#include "Asset/UTexture.h"
#include "Core/Asset/FAssetReference.h"

struct FMaterialTextureMap {
public:
    ReflectTypes(FMaterialTextureMap);

public:
    ReflectStruct(FMaterialTextureMap);

    void SetTexture(FAssetHandle Texture);
    void SetTextureReference(const FAssetReference& Texture);
    void Resolve(const IAssetResolver* Resolver);

public:
    FAssetHandle GetTexture() const;
    void Serialize(FArchive& Archive);

private:
    ReflectAccessor(mTexture, GetTexture, SetTexture, "Texture", EPropertyFlags::Edit | EPropertyFlags::Asset, ReflectAsset(UTexture));

    FAssetReference mTexture{};

    ReflectMembers();
};

struct FSurfaceParameters {
public:
    ReflectTypes(FSurfaceParameters);

public:
    ReflectStruct(FSurfaceParameters);

public:
    ReflectProperty(mAmbient, "Ambient", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Color, ReflectSerializer(mAmbient), ReflectEditor("Material", "Ambient"));

    FVector3 mAmbient{0.0f, 0.0f, 0.0f};
    ReflectProperty(mDiffuse, "Diffuse", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Color, ReflectSerializer(mDiffuse), ReflectEditor("Material", "Diffuse"));

    FVector4 mDiffuse{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectProperty(mSpecular, "Specular", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Color, ReflectSerializer(mSpecular), ReflectEditor("Material", "Specular"));

    FVector3 mSpecular{0.0f, 0.0f, 0.0f};
    ReflectProperty(mEmissive, "Emissive", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Color, ReflectSerializer(mEmissive), ReflectEditor("Material", "Emissive"));

    FVector3 mEmissive{0.0f, 0.0f, 0.0f};
    ReflectProperty(mTransmissionFilter, "TransmissionFilter", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mTransmissionFilter), ReflectEditor("Material", "Transmission Filter"), ReflectRange(0.0f, 1.0f), ReflectSpeed(0.01f));

    FVector3 mTransmissionFilter{1.0f, 1.0f, 1.0f};
    ReflectProperty(mShininess, "Shininess", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mShininess), ReflectEditor("Material", "Shininess"), ReflectRange(0.0f, 1000.0f), ReflectSpeed(1.0f));

    float mShininess{0.0f};
    ReflectProperty(mRefractionIndex, "RefractionIndex", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mRefractionIndex), ReflectEditor("Material", "Refraction Index"), ReflectRange(0.0f, 10.0f), ReflectSpeed(0.01f));

    float mRefractionIndex{1.0f};
    ReflectProperty(mSharpness, "Sharpness", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mSharpness), ReflectEditor("Material", "Sharpness"), ReflectRange(0.0f, 1000.0f), ReflectSpeed(1.0f));

    float mSharpness{60.0f};
    ReflectProperty(mIlluminationModel, "IlluminationModel", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mIlluminationModel), ReflectEditor("Material", "Illumination Model"), ReflectSpeed(1.0f));

    Int32 mIlluminationModel{2};
    ReflectProperty(mBDissolveHalo, "BDissolveHalo", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mBDissolveHalo), ReflectEditor("Material", "Dissolve Halo"));

    bool mBDissolveHalo{false};
    ReflectProperty(mAmbientTexture, "AmbientTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mAmbientTexture), ReflectEditor("Textures", "Ambient"));

    FMaterialTextureMap mAmbientTexture{};
    ReflectProperty(mDiffuseTexture, "DiffuseTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mDiffuseTexture), ReflectEditor("Textures", "Diffuse"));

    FMaterialTextureMap mDiffuseTexture{};
    ReflectProperty(mSpecularTexture, "SpecularTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mSpecularTexture), ReflectEditor("Textures", "Specular"));

    FMaterialTextureMap mSpecularTexture{};
    ReflectProperty(mEmissiveTexture, "EmissiveTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mEmissiveTexture), ReflectEditor("Textures", "Emissive"));

    FMaterialTextureMap mEmissiveTexture{};
    ReflectProperty(mTransmissionTexture, "TransmissionTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mTransmissionTexture), ReflectEditor("Textures", "Transmission"));

    FMaterialTextureMap mTransmissionTexture{};
    ReflectProperty(mShininessTexture, "ShininessTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mShininessTexture), ReflectEditor("Textures", "Shininess"));

    FMaterialTextureMap mShininessTexture{};
    ReflectProperty(mOpacityTexture, "OpacityTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mOpacityTexture), ReflectEditor("Textures", "Opacity"));

    FMaterialTextureMap mOpacityTexture{};
    ReflectProperty(mBumpTexture, "BumpTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mBumpTexture), ReflectEditor("Textures", "Bump"));

    FMaterialTextureMap mBumpTexture{};
    ReflectProperty(mNormalTexture, "NormalTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mNormalTexture), ReflectEditor("Textures", "Normal"));

    FMaterialTextureMap mNormalTexture{};
    ReflectProperty(mDisplacementTexture, "DisplacementTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mDisplacementTexture), ReflectEditor("Textures", "Displacement"));

    FMaterialTextureMap mDisplacementTexture{};
    ReflectProperty(mDecalTexture, "DecalTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mDecalTexture), ReflectEditor("Textures", "Decal"));

    FMaterialTextureMap mDecalTexture{};
    ReflectProperty(mReflectionTexture, "ReflectionTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mReflectionTexture), ReflectEditor("Textures", "Reflection"));

    FMaterialTextureMap mReflectionTexture{};

    ReflectMembers();
};
