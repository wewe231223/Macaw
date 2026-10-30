/**
 * @file FMaterialGPUData.h
 * @brief GPU 머티리얼 버퍼의 고정 슬롯 크기와 원시 바이트 저장 형식을 정의합니다.
 */

#pragma once

#include "Core/Common.h"

#include <array>
#include <cstddef>

/**
 * @brief GPU 머티리얼 슬롯 하나의 바이트 크기이며 구조화 버퍼의 원소 간격입니다.
 */
constexpr Uint32 MaterialGpuStride{128};

/**
 * @brief 머티리얼 셰이더 데이터를 담는 128바이트 고정 저장 공간입니다.
 * @note 각 바이트의 의미는 머티리얼 레이아웃과 셰이더가 결정합니다.
 */
struct FMaterialGPUSlot {
    /**
     * @brief 0으로 초기화한 머티리얼 원시 데이터입니다.
     */
    std::array<std::byte, MaterialGpuStride> mData{};
};

static_assert(sizeof(FMaterialGPUSlot) == MaterialGpuStride);
