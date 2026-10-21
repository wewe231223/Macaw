#pragma once

#include "Core/Asset/FAssetPath.h"
#include "Core/Base/FAssetHandle.h"
#include "Core/Base/FGuid.h"

class IAssetResolver;

class FAssetReference {
public:
    FAssetReference() = default;
    FAssetReference(const FGuid& Guid, const FAssetPath& Path);

public:
    FAssetHandle GetHandle() const;
    const FGuid& GetGuid() const;
    const FAssetPath& GetPath() const;
    bool IsEmpty() const;
    void Assign(FAssetHandle Handle, const IAssetResolver* Resolver);
    FAssetHandle Resolve(const IAssetResolver* Resolver);
    void RefreshIdentity(const IAssetResolver* Resolver);

private:
    FAssetHandle mHandle{};
    FGuid mGuid{};
    FAssetPath mPath{};
};
