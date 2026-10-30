#pragma once

#include "Core/Math/FVector4.h"
#include "Engine/Import/FMeshImportTexture.h"

class FMeshImportMaterial {
public:
    ReflectTypes(FMeshImportMaterial);

public:
    FMeshImportMaterial() = default;
    FMeshImportMaterial(FString Name, const FVector4& BaseColor, bool TwoSided, FMeshImportTexture BaseColorTexture, FMeshImportTexture NormalMapTexture);

public:
    ReflectStruct(FMeshImportMaterial);

    const FString& GetName() const;
    const FVector4& GetBaseColor() const;
    bool IsTwoSided() const;

    const FMeshImportTexture& GetBaseColorTexture() const;
    const FMeshImportTexture& GetNormalMapTexture() const;

private:
    ReflectMembers();

private:
    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mBaseColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectProperty(mBaseColor, "BaseColor", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mTwoSided{};
    ReflectProperty(mTwoSided, "TwoSided", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FMeshImportTexture mBaseColorTexture{};
    ReflectProperty(mBaseColorTexture, "BaseColorTexture", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FMeshImportTexture mNormalMapTexture{};
    ReflectProperty(mNormalMapTexture, "NormalMapTexture", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
