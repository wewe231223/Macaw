#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using Int32 = std::int32_t;
using Uint8 = std::uint8_t;
using Uint32 = std::uint32_t;
using Uint64 = std::uint64_t;
using FString = std::string;

template <typename T>
using TArray = std::vector<T>;

template <typename K, typename V, typename H = std::hash<K>>
using TMap = std::unordered_map<K, V, H>;
