#pragma once

#include "Core/Common.h"

struct FSceneHandle {
    bool IsValid() const;
    bool operator==(const FSceneHandle& Other) const;

    Uint64 mId{};
    Uint64 mGeneration{};
};
