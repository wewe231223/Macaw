/**
 * @file STL.h
 * @brief 엔진 메모리 추적 할당자를 사용하는 문자열과 STL 컨테이너 별칭을 제공합니다.
 */

#pragma once
#include "Core/Memory/TEngineAllocator.h"

// 1. C/C++ 표준 수학 라이브러리
#include <cmath>
#include <cstdlib>

// STL 컨테이너 및 메모리
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <functional>
#include <array>
#include <algorithm>
#include <optional>

// ----------------------------------------------------
// [ 언리얼 엔진(UE) 스타일 Aliasing ]
// ----------------------------------------------------

// 문자열
/**
 * @brief String 메모리 태그로 동적 저장 공간을 추적하는 char 문자열입니다.
 */
using FString = std::basic_string<char, std::char_traits<char>, TEngineAllocator<char, Memory::EMemoryTag::String>>;

// 동적 배열
/**
 * @brief Container 메모리 태그를 사용하는 연속 저장 동적 배열입니다.
 * @tparam T 원소 타입입니다.
 */
template <typename T>
using TArray = std::vector<T, TEngineAllocator<T, Memory::EMemoryTag::Container>>;

// 맵/셋
/**
 * @brief Container 메모리 태그를 사용하는 해시 기반 키·값 컨테이너입니다.
 * @tparam Key std::hash와 std::equal_to로 처리할 키 타입입니다.
 * @tparam Value 저장할 값 타입입니다.
 */
template <typename Key, typename Value>
using TMap = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>, TEngineAllocator<std::pair<const Key, Value>, Memory::EMemoryTag::Container>>;

/**
 * @brief Container 메모리 태그를 사용하는 중복 없는 해시 집합입니다.
 * @tparam T std::hash와 std::equal_to로 처리할 원소 타입입니다.
 */
template <typename T>
using TSet = std::unordered_set<T, std::hash<T>, std::equal_to<T>, TEngineAllocator<T, Memory::EMemoryTag::Container>>;

// 힙 할당이 없으므로 Allocator 불필요
/**
 * @brief N개의 원소를 객체 내부에 저장하는 고정 크기 배열입니다.
 * @tparam T 원소 타입입니다.
 * @tparam N 컴파일 시 결정되는 원소 개수입니다.
 */
template <typename T, std::size_t N>
using TFixedArray = std::array<T, N>;
