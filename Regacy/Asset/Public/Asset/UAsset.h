#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "CoreUObject/UObject.h"
#include <filesystem>
#include "Core/Archive/FArchive.h"

class UAsset : public UObject {
public:
    ReflectTypes(UAsset, UObject);

public:
    UAsset();
    virtual ~UAsset();

    UAsset(const UAsset&) = delete;
    UAsset& operator=(const UAsset&) = delete;

    UAsset(UAsset&&) noexcept = default;
    UAsset& operator=(UAsset&&) noexcept = default;

public:
    ReflectClass(UAsset, EClassFlags::None);

    void SetAssetName(const FString& InName);

    FString GetAssetName() const;

protected:
    bool Initialize(const std::filesystem::path& InAssetPath);

    virtual void Serialize(FArchive& Ar) override;

protected:
    ReflectReadOnlyProperty(mAssetPath, "AssetPath", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mAssetPath));
    std::filesystem::path mAssetPath{};
    ReflectReadOnlyProperty(mAssetName, "AssetName", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mAssetName));
    FString mAssetName{};

    ReflectMembers();
};
