#pragma once

#include "../Base/UObject.h"
#include "FAssetHandle.h"
#include "FMaterialBuffer.h"
#include "UMaterial.h"

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
    bool Initialize(ID3D11Device* Device, uint32 MaxMaterialCount = 4096);

    template<typename T, typename... Args> requires std::is_base_of_v<UObject, T>
    inline FAssetHandle EmplaceAsset(ID3D11Device* Device, EAssetType Type, FString Name, Args&&... args) {
        std::unique_ptr<T> NewAsset = std::make_unique<T>();

        if (!NewAsset->Initialize(Device, std::forward<Args>(args)...)) {
            return {};
        }

        if constexpr (std::is_base_of_v<UMaterial, T>) {
            const uint32 GPUIndex = MaterialBuffer.RegisterMaterial(NewAsset.get());

            if (GPUIndex == UINT32_MAX) {
                return {};
            }
        }

        auto& Container = Assets[Type];

        size_t NewAssetIndex = Container.size();

        auto It = std::find_if(Container.begin(), Container.end(), [](const TPair<FAssetHandle, std::unique_ptr<UObject>>& Pair) {
            return Pair.second == nullptr;
            });

        FAssetHandle NewHandle{};

        if (It != Container.end()) {
            NewAssetIndex = static_cast<size_t>(std::distance(Container.begin(), It));

            NewHandle = FAssetHandle{
                static_cast<uint32>(NewAssetIndex),
                It->first.Generation + 1
            };

            Container[NewAssetIndex] = TPair<FAssetHandle, std::unique_ptr<UObject>>{
                NewHandle,
                std::move(NewAsset)
            };
        }
        else {
            NewHandle = FAssetHandle{
                static_cast<uint32>(NewAssetIndex),
                0
            };

            Container.emplace_back(NewHandle, std::move(NewAsset));
        }

        AssetNameToHandle[Type][Name] = NewHandle;

        return NewHandle;
    }

    FAssetHandle GetAsset(EAssetType Type, FString Name) const;

    bool RemoveAsset(EAssetType Type, FAssetHandle Handle);

    template<typename T> requires std::is_base_of_v<UObject, T>
    inline T* ResolveAsset(EAssetType Type, FAssetHandle Handle) {
        auto& Container = Assets[Type];

        if (Handle.ID >= Container.size()) {
            return nullptr;
        }

        auto& Entry = Container[Handle.ID];

        if (Entry.first.Generation != Handle.Generation || Entry.second == nullptr) {
            return nullptr;
        }

        return static_cast<T*>(Entry.second.get());
    }

    template<typename T, typename Func>
    void ModifyAsset(EAssetType Type, FAssetHandle Handle, Func&& Modifier) {
        T* Asset = ResolveAsset<T>(Type, Handle);

        if (Asset == nullptr) {
            return;
        }

        std::invoke(std::forward<Func>(Modifier), *Asset);
    }


    FMaterialBuffer& GetMaterialBuffer() {
        return MaterialBuffer;
    }

    const FMaterialBuffer& GetMaterialBuffer() const {
        return MaterialBuffer;
    }

private:
    TFixedArray<TArray<TPair<FAssetHandle, std::unique_ptr<UObject>>>, EAssetType::END> Assets{};
    TFixedArray<TMap<FString, FAssetHandle>, EAssetType::END> AssetNameToHandle{};

    FMaterialBuffer MaterialBuffer{};
};