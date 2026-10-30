/**
 * @file Types.h
 * @brief 모듈 공통 정수, 문자열 및 컨테이너 별칭을 정의합니다.
 * @ingroup Core
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Core {

    /**
     * @brief 32비트 부호 있는 정수 타입입니다.
     */
    using Int32 = std::int32_t;

    /**
     * @brief 8비트 부호 없는 정수 타입입니다.
     */
    using Uint8 = std::uint8_t;

    /**
     * @brief 32비트 부호 없는 정수 타입입니다.
     */
    using Uint32 = std::uint32_t;

    /**
     * @brief 64비트 부호 없는 정수 타입입니다.
     */
    using Uint64 = std::uint64_t;

    /**
     * @brief UTF-8 텍스트에 사용하는 표준 문자열 타입입니다.
     */
    using FString = std::string;

    /**
     * @brief 동적으로 크기가 변하는 연속 배열입니다.
     * @tparam T 컨테이너의 값, 키 또는 해시 타입입니다.
     */
    template <typename T>
    using TArray = std::vector<T>;

    /**
     * @brief 키와 값 및 선택한 해시 함수로 구성하는 해시 맵입니다.
     * @tparam K 컨테이너의 값, 키 또는 해시 타입입니다.
     * @tparam V 컨테이너의 값, 키 또는 해시 타입입니다.
     * @tparam H 컨테이너의 값, 키 또는 해시 타입입니다.
     */
    template <typename K, typename V, typename H = std::hash<K>>
    using TMap = std::unordered_map<K, V, H>;

}
