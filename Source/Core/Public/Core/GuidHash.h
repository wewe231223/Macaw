#pragma once

#include "Core/Guid.h"

struct FGuidHash {
public:
    std::size_t operator()(const FGuid& Value) const noexcept;
};
