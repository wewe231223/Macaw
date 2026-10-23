/**
 * @file FAssetHandle.h
 * @brief 에셋 슬롯 ID와 세대 값으로 구성된 런타임 핸들을 정의한다.
 */
#pragma once

#include "Core/CoreMinimal.h"
#include "Core/Common.h"

#include <limits>
class FArchive;

/**
 * @brief 레지스트리의 에셋 슬롯을 ID와 세대 값으로 식별한다.
 * @details 슬롯을 재사용할 때 세대 값을 변경하여 이전 핸들과 구분한다. 기본 초기화된 핸들은 ID가 Uint32의 최댓값이고 세대가 0인 유효하지 않은 핸들이다.
 */
struct FAssetHandle {
    /** @brief 레지스트리 내 에셋 슬롯 인덱스. Uint32의 최댓값은 슬롯이 지정되지 않았음을 나타낸다. */
    Uint32 mId{std::numeric_limits<Uint32>::max()};
    /** @brief 같은 슬롯을 재사용할 때 이전 핸들과 구분하는 세대 값. 기본값은 0이다. */
    Uint32 mGeneration{0};

    /**
     * @brief 두 핸들의 슬롯 ID와 세대 값이 모두 같은지 비교한다.
     * @param Other 비교할 에셋 핸들.
     * @return mId와 mGeneration이 각각 같으면 true, 하나라도 다르면 false.
     */
    bool operator==(const FAssetHandle& Other) const;

    /**
     * @brief 두 핸들의 슬롯 ID 또는 세대 값이 다른지 비교한다.
     * @param Other 비교할 에셋 핸들.
     * @return mId와 mGeneration 중 하나라도 다르면 true, 모두 같으면 false.
     */
    bool operator!=(const FAssetHandle& Other) const;

    /**
     * @brief 핸들에 슬롯 ID가 지정되어 있는지 불리언 값으로 확인한다.
     * @return mId가 Uint32의 최댓값이 아니면 true, 최댓값이면 false.
     * @details 세대 값이나 레지스트리의 에셋 존재 여부는 검사하지 않는다. 삭제된 에셋의 이전 핸들도 슬롯 ID가 남아 있으면 true로 평가된다.
     */
    operator bool() const;

    /**
     * @brief 슬롯 ID와 세대 값을 아카이브로 직렬화한다.
     * @param[in,out] Archive 핸들 정보를 읽거나 기록할 아카이브.
     * @details mId를 ID 필드로, mGeneration을 Generation 필드로 순서대로 직렬화한다. 로딩 시 멤버 값을 읽어 갱신하고 저장 시 현재 값을 기록한다.
     */
    void Serialize(FArchive& Archive);
};
