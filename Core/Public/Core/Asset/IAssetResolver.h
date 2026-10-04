#pragma once
#include "Core/Base/FAssetHandle.h"
#include "Core/Base/FGuid.h"
#include "Core/Asset/FAssetPath.h"

class IAssetResolver {
public:
    virtual ~IAssetResolver() = default;

public:
    virtual FAssetHandle FindAsset(const FAssetPath& AssetPath) const = 0;
    virtual FAssetHandle FindAsset(const FGuid& PersistentGuid) const = 0;
    virtual const FAssetPath* GetAssetPath(FAssetHandle Handle) const = 0;
    virtual const FGuid* GetAssetGuid(FAssetHandle Handle) const = 0;
};
