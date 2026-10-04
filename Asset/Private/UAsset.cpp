#include "pch.h"
#include "Asset/UAsset.h"

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

bool UAsset::Initialize(const std::filesystem::path& InAssetPath) {
    if (InAssetPath.empty()) {
        return false;
    }

    mAssetPath = InAssetPath;
    return true;
}
