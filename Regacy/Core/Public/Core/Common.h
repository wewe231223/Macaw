/**
 * @file Common.h
 * @brief 공용 STL 및 기반 타입을 포함하고 정수와 부동소수점 타입 별칭을 정의한다.
 */

#pragma once
#include "Core/STL.h"
#include <cmath>
#include "Core/Types.h"

// -----------------------------------------------------------------
// 1. 고정 크기 정수형 타입 (Fixed-Width Integer Types)
// -----------------------------------------------------------------
/**
 * @brief 부호 있는 8비트 정수 타입.
 */
using Int8 = std::int8_t;

/**
 * @brief 부호 있는 16비트 정수 타입.
 */
using Int16 = std::int16_t;

/**
 * @brief 부호 있는 32비트 정수 타입.
 */
using Int32 = std::int32_t;

/**
 * @brief 부호 있는 64비트 정수 타입.
 */
using Int64 = std::int64_t;

/**
 * @brief 부호 없는 8비트 정수 타입.
 */
using Uint8 = std::uint8_t;

/**
 * @brief 부호 없는 16비트 정수 타입.
 */
using Uint16 = std::uint16_t;

/**
 * @brief 부호 없는 32비트 정수 타입.
 */
using Uint32 = std::uint32_t;

/**
 * @brief 부호 없는 64비트 정수 타입.
 */
using Uint64 = std::uint64_t;

// -----------------------------------------------------------------
// 2. 부동소수점 타입 (Floating-Point Types)
// -----------------------------------------------------------------
/**
 * @brief 단정도 부동소수점 타입 float의 별칭.
 */
using Float32 = float;

/**
 * @brief 배정도 부동소수점 타입 double의 별칭.
 */
using Float64 = double;
