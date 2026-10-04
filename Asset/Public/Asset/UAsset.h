#pragma once
#include "CoreUObject/UObject.h"
#include <filesystem>
#include "Core/Archive/FArchive.h"

class UAsset : public UObject {
public:
    UAsset() = default;
    virtual ~UAsset() = default;

    UAsset(const UAsset&) = delete;
    UAsset& operator=(const UAsset&) = delete;

    UAsset(UAsset&&) noexcept = default;
    UAsset& operator=(UAsset&&) noexcept = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UAsset, UObject);

    void SetAssetName(const FString& InName);

    FString GetAssetName() const;

protected:
    bool Initialize(const std::filesystem::path& InAssetPath);

    virtual void Serialize(FArchive& Ar) override;

protected:
    std::filesystem::path mAssetPath{};
    FString mAssetName{};
};
