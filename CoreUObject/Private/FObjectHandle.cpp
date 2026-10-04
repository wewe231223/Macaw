#include "pch.h"
#include "CoreUObject/FObjectHandle.h"

bool FObjectHandle::IsValid() const {
    return mIndex != InvalidIndex;
}
