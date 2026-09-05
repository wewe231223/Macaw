#include "PCH.h"
#include "FAssetRegistry.h"
#include "../../ErrorHandler.h"

FAssetHandle FAssetRegistry::GetAsset(EAssetType Type, FString Name) {
	auto it = AssetNameToHandle[Type].find(Name);
	if (it != AssetNameToHandle[Type].end()) {
		return it->second;
	}
	ErrorHandler::Report("[ AssetRegistry ]", "Asset not found, Invalid Handle Returned for : " + Name, ErrorHandler::EErrorLevel::Warning);

	return FAssetHandle();
}
