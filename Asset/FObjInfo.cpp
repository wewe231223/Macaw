#include "pch.h"
#include "FObjInfo.h"

bool FFaceVertexKey::operator==(const FFaceVertexKey& Other) const noexcept {
    return mPositionIndex == Other.mPositionIndex && mUvIndex == Other.mUvIndex && mNormalIndex == Other.mNormalIndex;
}

std::size_t FFaceVertexKeyHash::operator()(const FFaceVertexKey& Key) const noexcept {
    std::size_t Hash{std::hash<Int32>{}(Key.mPositionIndex)};
    Hash = Hash * 31 + std::hash<Int32>{}(Key.mUvIndex);
    Hash = Hash * 31 + std::hash<Int32>{}(Key.mNormalIndex);
    return Hash;
}
