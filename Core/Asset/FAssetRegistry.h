#pragma once 

#include "../Base/UObject.h"
#include "FAssetHandle.h"
#include <d3d11.h>

enum EAssetType {
	Mesh = 0,
	Pipeline = 1,
	Material = 2,
	END
};

class FAssetRegistry {
public:
	FAssetRegistry() = default;
	~FAssetRegistry() = default;

	FAssetRegistry(const FAssetRegistry&) = delete;
	FAssetRegistry& operator=(const FAssetRegistry&) = delete;

	FAssetRegistry(FAssetRegistry&&) = delete;
	FAssetRegistry& operator=(FAssetRegistry&&) = delete;

public:
	template<typename T, typename... Args> requires std::is_base_of_v<UObject, T> 
	inline void EmplaceAsset(ID3D11Device* Device, EAssetType Type, FString Name, Args&&... args) {
		std::unique_ptr<T> NewAsset = std::make_unique<T>();
		NewAsset->Initialize(Device, std::forward<Args>(args)...);

		size_t NewAssetIndex = Assets[Type].size();
		auto& cont = Assets[Type];

		auto it = std::find_if(cont.begin(), cont.end(), [&](const TPair<FAssetHandle, std::unique_ptr<UObject>>& pair) {
			return pair.second == nullptr; 
			}
		);	
			
		if (it != cont.end()) {
			NewAssetIndex = std::distance(cont.begin(), it);
			FAssetHandle NewHandle{ static_cast<uint32>(NewAssetIndex), it->first.Generation + 1 };
			cont[NewAssetIndex] = TPair<FAssetHandle, std::unique_ptr<UObject>>{ NewHandle, std::move(NewAsset) };
			AssetNameToHandle[Type][Name] = NewHandle;
		}
		else {
			cont.emplace_back(FAssetHandle{ static_cast<uint32>(NewAssetIndex), 0 }, std::move(NewAsset));
			AssetNameToHandle[Type][Name] = FAssetHandle{ static_cast<uint32>(NewAssetIndex), 0 };
		}
	} 

	FAssetHandle GetAsset(EAssetType Type, FString Name);

public:
	TFixedArray<TArray<TPair<FAssetHandle, std::unique_ptr<UObject>>>, EAssetType::END> Assets{};
	TFixedArray<TMap<FString, FAssetHandle>, EAssetType::END> AssetNameToHandle{};
};