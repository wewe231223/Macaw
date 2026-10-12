#pragma once
#include "CoreUObject/UObject.h"
#include <filesystem>
#include "Core/Archive/FArchive.h"
#include "Asset/UAsset.generated.h"

UCLASS()
class UAsset : public UObject {
public:
    GENERATED_TYPES(UAsset)

public:
    UAsset();
    virtual ~UAsset();

    UAsset(const UAsset&) = delete;
    UAsset& operator=(const UAsset&) = delete;

    UAsset(UAsset&&) noexcept = default;
    UAsset& operator=(UAsset&&) noexcept = default;

public:
    GENERATED_BODY(UAsset)

    void SetAssetName(const FString& InName);

    FString GetAssetName() const;

protected:
    bool Initialize(const std::filesystem::path& InAssetPath);

    virtual void Serialize(FArchive& Ar) override;

protected:
    std::filesystem::path mAssetPath{};
    FString mAssetName{};
};
