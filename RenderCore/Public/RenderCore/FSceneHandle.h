/**
 * @file FSceneHandle.h
 * @brief 렌더 씬의 식별 ID와 세대 핸들을 정의합니다.
 */
#pragma once

#include "Core/Common.h"

/**
 * @brief 원자적 카운터에서 다음 렌더 씬 ID를 할당합니다.
 * @return 1부터 증가하는 ID입니다.
 * @note 카운터 갱신은 원자적이지만 다른 씬 데이터의 동기화를 제공하지 않으며 정수 순환은 별도로 처리하지 않습니다.
 */
Uint64 AllocateRenderSceneId();

/**
 * @brief 씬 ID와 세대로 현재 렌더 씬 상태를 구별하는 비소유 핸들입니다.
 */
struct FSceneHandle {
    /**
     * @brief 씬 ID와 세대가 모두 0이 아닌지 검사합니다.
     * @return 두 값이 모두 0이 아니면 true입니다.
     * @note 실제 씬의 존재 여부나 현재 세대 일치 여부는 검사하지 않습니다.
     */
    bool IsValid() const;
    /**
     * @brief 씬 ID와 세대를 함께 비교합니다.
     * @param Other 비교할 핸들입니다.
     * @return 두 값이 모두 일치하면 true입니다.
     */
    bool operator==(const FSceneHandle& Other) const;

    /**
     * @brief 렌더 씬을 식별하는 ID입니다. 0은 무효 값입니다.
     */
    Uint64 mId{};
    /**
     * @brief 씬 재설정에 따른 오래된 갱신 요청을 구별하는 세대입니다. 0은 무효 값입니다.
     */
    Uint64 mGeneration{};
};
