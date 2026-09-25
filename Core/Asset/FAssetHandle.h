#pragma once

#include "Serialize/FArchive.h"

struct FAssetHandle {
    Uint32 mId{std::numeric_limits<Uint32>::max()};
    Uint32 mGeneration{0};

    bool operator==(const FAssetHandle& Other) const;

    bool operator!=(const FAssetHandle& Other) const;

    operator bool() const;

    void Serialize(FArchive& Archive);
};
