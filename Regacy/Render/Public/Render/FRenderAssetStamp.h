/**
 * @file FRenderAssetStamp.h
 * @brief 에셋 서비스 세대와 객체 및 수정 상태로 GPU 캐시의 유효성을 구분합니다.
 */
#pragma once
#include "Asset/FAssetRegistry.h"
#include "CoreUObject/FObjectHandle.h"

/** @brief 서비스 교체와 에셋 수정 시 캐시를 무효화하는 비교 값입니다. */
struct FRenderAssetStamp {
    /** @brief 등록된 에셋 객체의 런타임 핸들입니다. 미등록 객체는 무효 핸들일 수 있습니다. */
    FObjectHandle mAssetHandle{};
    /** @brief 에셋별 렌더 데이터 수정 번호입니다. */
    Uint64 mRevision{};
    /** @brief 상태 생성 시점의 전역 에셋 서비스 세대입니다. */
    Uint64 mRegistryGeneration{FAssetRegistry::GetGeneration()};

    /** @brief 객체 핸들, 수정 번호와 서비스 세대가 모두 같은지 검사합니다. */
    bool operator==(const FRenderAssetStamp&) const = default;
};
