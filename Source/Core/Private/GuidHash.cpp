/**
 * @file GuidHash.cpp
 * @brief GUID의 해시 컨테이너 연동을 제공합니다.
 * @ingroup Core
 */

#include "Core/GuidHash.h"

/**
 * @brief GUID의 해시를 계산합니다.
 */
std::size_t FGuidHash::operator()(const FGuid& Value) const noexcept {
    std::size_t Hash{1469598103934665603ull};

    for (Uint8 Byte : Value.mBytes) {
        Hash = (Hash ^ Byte) * 1099511628211ull;
    }

    return Hash;
}
