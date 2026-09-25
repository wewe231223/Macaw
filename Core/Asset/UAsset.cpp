#include "pch.h"
#include "UAsset.h"

void UAsset::Serialize(FArchive& Ar) {
    UObject::Serialize(Ar);
    Ar.Serialize("AssetName", mAssetName);

    FString PathStr{FString{mAssetPath.string()}};
    Ar.Serialize("AssetPath", PathStr);
}

void UAsset::SetAssetName(const FString& InName) {
    mAssetName = InName;
}

FString UAsset::GetAssetName() const {
    return mAssetName;
}

bool UAsset::Initialize(ID3D11Device* Device, const std::filesystem::path& InAssetPath) {
    if (Device == nullptr || InAssetPath.empty()) {
        return false;
    }

    mAssetPath = InAssetPath;
    return true;
}
