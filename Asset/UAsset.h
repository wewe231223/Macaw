#pragma once
#include <filesystem>
#include <d3d11.h>
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
    bool Initialize(ID3D11Device* Device, const std::filesystem::path& InAssetPath);

    virtual void Serialize(FArchive& Ar) override;

protected:
    std::filesystem::path mAssetPath{};
    FString mAssetName{};
};
