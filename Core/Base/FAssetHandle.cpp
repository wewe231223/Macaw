#include "pch.h"
#include "FAssetHandle.h"
#include "Core/Archive/FArchive.h"

void FAssetHandle::Serialize(FArchive& Archive) {
    Archive.Serialize("ID", mId);
    Archive.Serialize("Generation", mGeneration);
}
bool FAssetHandle::operator==(const FAssetHandle& Other) const {
    return mId == Other.mId && mGeneration == Other.mGeneration;
}

bool FAssetHandle::operator!=(const FAssetHandle& Other) const {
    return !(*this == Other);
}

FAssetHandle::operator bool() const {
    return mId != std::numeric_limits<Uint32>::max();
}
