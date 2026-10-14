/**
 * @file FObjectHandle.h
 * @brief UObject 등록 슬롯과 재사용 세대를 식별하는 핸들을 제공합니다.
 */
#pragma once

#include "Core/CoreMinimal.h"

#include <cstdint>
#include <limits>

/**
 * @brief 슬롯 인덱스와 세대로 등록 객체를 식별하는 비소유 핸들입니다.
 * @note 객체의 현재 생존 여부는 UObjectSystem::Resolve로 검사합니다.
 */
struct FObjectHandle {
    /**
     * @brief 미등록 상태를 나타내는 인덱스 값입니다.
     */
    static constexpr std::uint32_t InvalidIndex{std::numeric_limits<std::uint32_t>::max()};

    /**
     * @brief 객체 저장소의 슬롯 인덱스입니다.
     */
    std::uint32_t mIndex{InvalidIndex};
    /**
     * @brief 슬롯 재사용에 따른 오래된 참조를 구별하는 세대 값입니다.
     */
    std::uint32_t mGeneration{0};

    /**
     * @brief 인덱스가 InvalidIndex와 다른지 검사합니다.
     * @return 무효 인덱스가 아니면 true입니다.
     * @note 범위, 세대 일치 또는 객체 생존 여부는 검사하지 않습니다.
     */
    bool IsValid() const;

    /**
     * @brief 인덱스와 세대가 모두 같은지 비교합니다.
     * @return 두 저장 값이 모두 같으면 true입니다.
     */
    bool operator==(const FObjectHandle&) const = default;
};
