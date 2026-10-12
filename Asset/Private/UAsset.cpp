#include "pch.h"
#include "Asset/UAsset.h"

void UAsset::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
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

UAsset::UAsset() {
    InitializeClass(StaticClass());
}

UAsset::~UAsset() = default;
