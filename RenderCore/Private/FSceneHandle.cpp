#include "pch.h"
#include "RenderCore/FSceneHandle.h"

bool FSceneHandle::IsValid() const {
    return mId != 0 && mGeneration != 0;
}

bool FSceneHandle::operator==(const FSceneHandle& Other) const {
    return mId == Other.mId && mGeneration == Other.mGeneration;
}
