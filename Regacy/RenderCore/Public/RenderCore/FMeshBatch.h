/**
 * @file FMeshBatch.h
 * @brief 메시·머티리얼·파이프라인과 LOD별 인덱스 구간을 묶는 그리기 데이터를 정의합니다.
 */
#pragma once
#include "Core/CoreMinimal.h"
#include "Core/Base/FAssetHandle.h"
#include "RenderCore/FPrimitiveSceneProxy.h"

/**
 * @brief 하나의 머티리얼 그룹에 대응하는 메시 인덱스 구간입니다.
 */
struct FMeshBatchElement {
    /**
     * @brief 인덱스 버퍼에서 구간이 시작하는 원소 인덱스입니다. 바이트 오프셋이 아닙니다.
     */
    Uint32 mFirstIndex{};
    /**
     * @brief 현재 LOD에서 그릴 인덱스 수입니다.
     */
    Uint32 mIndexCount{};
    /**
     * @brief 적용할 머티리얼 그룹의 인덱스입니다.
     */
    Uint32 mMaterialSlotIndex{};
    /**
     * @brief LOD 감소 전 기준 인덱스 수이며 렌더링 통계 비교에 사용합니다.
     */
    Uint32 mOriginalIndexCount{};
};

/**
 * @brief 동일 메시·머티리얼·파이프라인과 LOD의 그리기 구간을 묶는 배치입니다.
 */
struct FMeshBatch {
    /**
     * @brief 그릴 메시 에셋 핸들입니다.
     */
    FAssetHandle mMeshHandle{};
    TArray<FMeshMaterialBinding> mMaterialOverrides{};
    /**
     * @brief 0부터 시작하는 메시 상세 수준입니다.
     */
    Uint32 mLODLevel{};
    /**
     * @brief 배치의 머티리얼 그룹별 인덱스 구간 목록입니다.
     */
    TArray<FMeshBatchElement> mElements{};
};
