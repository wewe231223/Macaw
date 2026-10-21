#include "CoreUObject/FObjectHandle.h"

bool FObjectHandle::IsValid() const {
    return mIndex != std::numeric_limits<Uint32>::max() && mGeneration != 0;
}

bool FObjectHandle::operator==(const FObjectHandle& Other) const {
    return mIndex == Other.mIndex && mGeneration == Other.mGeneration;
}
