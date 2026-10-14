#pragma once

#include <cstddef>
#include "Core/Stat/Stat.h"

namespace Memory {
    using EMemoryTag = Stat::EMemoryTag;

    void* Allocate(std::size_t Size, std::size_t Alignment, EMemoryTag Tag = EMemoryTag::Unknown);
    void Free(void* Ptr) noexcept;
}
