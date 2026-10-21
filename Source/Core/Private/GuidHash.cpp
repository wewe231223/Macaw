#include "Core/GuidHash.h"

std::size_t FGuidHash::operator()(const FGuid& Value) const noexcept {
    std::size_t Hash{1469598103934665603ull};

    for (Uint8 Byte : Value.mBytes) {
        Hash = (Hash ^ Byte) * 1099511628211ull;
    }

    return Hash;
}
