#pragma once

#include <cstddef>

struct FMemoryStats {
    std::size_t mLiveObjects{};
    std::size_t mCreatedObjects{};

    std::size_t mHeapBytes{};
    std::size_t mPeakHeapBytes{};
};
