#pragma once

#include "Core/Common.h"

#include <array>
#include <cstddef>

constexpr Uint32 MaterialGpuStride{128};

struct FMaterialGPUSlot {
    std::array<std::byte, MaterialGpuStride> mData{};
};

static_assert(sizeof(FMaterialGPUSlot) == MaterialGpuStride);
