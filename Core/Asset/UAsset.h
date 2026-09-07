#pragma once 
#include <filesystem>
#include <d3d11.h>

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

	virtual void Initialize(ID3D11Device* device, const std::filesystem::path& metaData) { AssetMetaDataPath = metaData; };

	void SetAssetName(const FString& InName) { AssetName = InName; }

protected:
	virtual void Serialize(FArchive& Ar) override;

protected:
	std::filesystem::path AssetMetaDataPath{};
	FString AssetName{}; 
};