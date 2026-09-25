#pragma once

#include <cstdint>
#include <limits>

struct FObjectHandle {
    static constexpr std::uint32_t InvalidIndex{std::numeric_limits<std::uint32_t>::max()};

    std::uint32_t mIndex{InvalidIndex};
    std::uint32_t mGeneration{0};

    bool IsValid() const;

    bool operator==(const FObjectHandle&) const = default;
};
