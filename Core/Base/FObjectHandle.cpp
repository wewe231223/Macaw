#include "pch.h"
#include "FObjectHandle.h"

bool FObjectHandle::IsValid() const {
    return mIndex != InvalidIndex;
}
