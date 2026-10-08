#pragma once
#include "CoreUObject/FObjectHandle.h"

struct FRenderAssetStamp {
    FObjectHandle mAssetHandle{};
    Uint64 mRevision{};

    bool operator==(const FRenderAssetStamp&) const = default;
};
