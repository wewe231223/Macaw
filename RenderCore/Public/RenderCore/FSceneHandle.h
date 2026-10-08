#pragma once

#include "Core/Common.h"

Uint64 AllocateRenderSceneId();

struct FSceneHandle {
    bool IsValid() const;
    bool operator==(const FSceneHandle& Other) const;

    Uint64 mId{};
    Uint64 mGeneration{};
};
