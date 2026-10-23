/**
 * @file FWorldHandle.cpp
 * @brief 월드 핸들의 형식 검증과 식별 값 비교를 구현합니다.
 * @ingroup Engine
 */

#include "Engine/FWorldHandle.h"

/**
 * @brief 슬롯과 세대를 조합하여 핸들을 구성합니다.
 */
FWorldHandle::FWorldHandle(Uint32 Index, Uint64 Generation)
	: mIndex{Index},
	  mGeneration{Generation} {
}

/**
 * @brief 슬롯과 세대가 무효 기본값이 아닌지 검사합니다.
 */
bool FWorldHandle::IsValid() const {
    return mIndex != std::numeric_limits<Uint32>::max() && mGeneration != 0;
}

/**
 * @brief 월드 컨텍스트의 슬롯을 반환합니다.
 */
Uint32 FWorldHandle::GetIndex() const {
    return mIndex;
}

/**
 * @brief 월드 슬롯의 재사용을 구분하는 세대를 반환합니다.
 */
Uint64 FWorldHandle::GetGeneration() const {
    return mGeneration;
}

/**
 * @brief 슬롯과 세대가 모두 같은지 비교합니다.
 */
bool FWorldHandle::operator==(const FWorldHandle& Other) const {
    return mIndex == Other.mIndex && mGeneration == Other.mGeneration;
}
