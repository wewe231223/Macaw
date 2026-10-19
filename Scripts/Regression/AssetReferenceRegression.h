#pragma once

#include "Core/Asset/IAssetResolver.h"

class FReferenceResolver final : public IAssetResolver {
public:
    FReferenceResolver(FAssetHandle Handle, const FGuid& Guid, const FAssetPath& Path);

public:
    FAssetHandle FindAsset(const FAssetPath& Path) const override;
    FAssetHandle FindAsset(const FGuid& Guid) const override;
    const FAssetPath* GetAssetPath(FAssetHandle Handle) const override;
    const FGuid* GetAssetGuid(FAssetHandle Handle) const override;
    void SetAvailable(bool Available);
    Uint32 GetPathLookupCount() const;

private:
    FAssetHandle mHandle{};
    FGuid mGuid{};
    FAssetPath mPath{};
    bool mAvailable{true};
    mutable Uint32 mPathLookupCount{};
};
