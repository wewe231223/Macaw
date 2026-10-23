/**
 * @file FObjectHandle.cpp
 * @brief 슬롯 인덱스와 세대로 구성되어 재사용된 슬롯의 오참조를 막는 핸들입니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/FObjectHandle.h"

/**
 * @brief 슬롯 인덱스와 세대가 유효한 핸들 표현인지 검사합니다. 실제 객체 존재 여부는 레지스트리에서 확인해야 합니다.
 */
bool FObjectHandle::IsValid() const {
    return mIndex != std::numeric_limits<Uint32>::max() && mGeneration != 0;
}

/**
 * @brief 저장된 성분 또는 식별 상태가 정확히 같은지 검사합니다.
 */
bool FObjectHandle::operator==(const FObjectHandle& Other) const {
    return mIndex == Other.mIndex && mGeneration == Other.mGeneration;
}
