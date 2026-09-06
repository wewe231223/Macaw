#include "PCH.h"
#include "FAssetRegistry.h"
#include "../../ErrorHandler.h"

#include "FAssetRegistry.h"

bool FAssetRegistry::Initialize(ID3D11Device* Device, uint32 MaxMaterialCount) {
    return MaterialBuffer.Initialize(Device, MaxMaterialCount);
}
	ErrorHandler::Report("[ AssetRegistry ]", "Asset not found, Invalid Handle Returned for : " + Name, ErrorHandler::EErrorLevel::Warning);

FAssetHandle FAssetRegistry::GetAsset(EAssetType Type, FString Name) const {
    const auto& NameMap = AssetNameToHandle[Type];

    const auto It = NameMap.find(Name);

    if (It == NameMap.end()) {
        return {};
    }

    return It->second;
}

bool FAssetRegistry::RemoveAsset(EAssetType Type, FAssetHandle Handle) {
    auto& Container = Assets[Type];

    if (Handle.ID >= Container.size()) {
        return false;
    }

    auto& Entry = Container[Handle.ID];

    if (Entry.first.Generation != Handle.Generation || Entry.second == nullptr) {
        return false;
    }

    if (Type == EAssetType::Material) {
        UMaterial* Material = static_cast<UMaterial*>(Entry.second.get());
        MaterialBuffer.UnregisterMaterial(Material);
    }

    for (auto It = AssetNameToHandle[Type].begin(); It != AssetNameToHandle[Type].end();) {
        if (It->second == Handle) {
            It = AssetNameToHandle[Type].erase(It);
        }
        else {
            ++It;
        }
    }

    Entry.second.reset();

    return true;
}