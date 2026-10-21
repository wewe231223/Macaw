#pragma once

#include "Core/Types.h"

#include <limits>

struct FObjectHandle {
public:
    bool IsValid() const;
    bool operator==(const FObjectHandle& Other) const;

public:
    Uint32 mIndex{std::numeric_limits<Uint32>::max()};
    Uint32 mGeneration{};
};
