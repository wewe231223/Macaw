#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector4.h"

class FAssetManager;
class FMemoryArchive;
class UMaterial;

class FMaterialAssetData {
public:
    ReflectTypes(FMaterialAssetData);

public:
    FMaterialAssetData();
    explicit FMaterialAssetData(const UMaterial& Material);

public:
    ReflectStruct(FMaterialAssetData);

    void Serialize(FMemoryArchive& Archive);
    bool ApplyTo(UMaterial& Material, FString& Error) const;
    bool ValidateReferences(const FAssetManager& Assets, FString& Error) const;

private:
    ReflectMembers();

private:
    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FString mShaderPath{};
    ReflectProperty(mShaderPath, "ShaderPath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FVector4 mBaseColor{};
    ReflectProperty(mBaseColor, "BaseColor", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FString mBaseColorTexturePath{};
    ReflectProperty(mBaseColorTexturePath, "BaseColorTexturePath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FString mNormalMapTexturePath{};
    ReflectProperty(mNormalMapTexturePath, "NormalMapTexturePath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    float mSpecularStrength{};
    ReflectProperty(mSpecularStrength, "SpecularStrength", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    float mShininess{};
    ReflectProperty(mShininess, "Shininess", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    float mNormalStrength{};
    ReflectProperty(mNormalStrength, "NormalStrength", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mTwoSided{};
    ReflectProperty(mTwoSided, "TwoSided", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
