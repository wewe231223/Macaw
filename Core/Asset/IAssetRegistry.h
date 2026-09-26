#pragma once

#include "Core/Base/FAssetHandle.h"
#include "Core/Base/FGuid.h"
#include "Core/Base/UObject.h"
#include "Core/Asset/FAssetPath.h"
#include <type_traits>

class IAssetRegistry {
public:
    virtual ~IAssetRegistry() = default;

public:
    virtual FAssetHandle FindAsset(const FAssetPath& AssetPath) const = 0;
    virtual FAssetHandle FindAsset(const FGuid& PersistentGuid) const = 0;
    virtual const FAssetPath* GetAssetPath(FAssetHandle Handle) const = 0;
    virtual const FGuid* GetAssetGuid(FAssetHandle Handle) const = 0;
    virtual TArray<FAssetHandle> GetAssetHandles(const FTypeInfo& AssetType) const = 0;

    template <typename T> requires std::is_base_of_v<UObject, T> const T* ResolveAsset(FAssetHandle Handle) const;

    virtual FAssetHandle EnsureDefaultStaticMeshMaterial() const = 0;
    virtual FAssetHandle EnsureDefaultStaticMeshPipeline() const = 0;

private:
    virtual const UObject* ResolveAssetObject(FAssetHandle Handle) const = 0;
};

template <typename T> requires std::is_base_of_v<UObject, T> const T* IAssetRegistry::ResolveAsset(FAssetHandle Handle) const {
    const UObject* Asset{ResolveAssetObject(Handle)};
    if (Asset == nullptr || !Asset->GetTypeInfo()->IsA(T::StaticTypeInfo())) {
        return nullptr;
    }

    return static_cast<const T*>(Asset);
}
