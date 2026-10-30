/**
 * @file FStaticPrimitiveDrawInterface.h
 * @brief 정적 메시 프록시가 LOD와 인덱스 구간을 조회하고 배치를 제출하는 인터페이스입니다.
 */

#pragma once
#include "RenderCore/FMeshBatch.h"

/**
 * @brief 에셋 접근 구현과 정적 메시 배치 생성을 연결하는 추상 인터페이스입니다.
 */
class FStaticPrimitiveDrawInterface {
public:
    /**
     * @brief 파생 그리기 인터페이스를 기반 포인터로 해제할 수 있게 합니다.
     */
    virtual ~FStaticPrimitiveDrawInterface();

public:
    /**
     * @brief 메시에서 조회 가능한 LOD 수를 반환합니다.
     * @param MeshHandle 조회할 메시 에셋 핸들입니다.
     * @return 구현이 제공하는 LOD 수입니다.
     */
    virtual Uint32 GetMeshLODCount(FAssetHandle MeshHandle) const = 0;

    /**
     * @brief 메시의 지정 LOD에 대응하는 그리기 구간을 조회합니다.
     * @param MeshHandle 조회할 메시 에셋 핸들입니다.
     * @param LODLevel 0부터 시작하는 상세 수준입니다.
     * @param[out] OutElements 머티리얼 그룹별 인덱스 구간 결과입니다.
     */
    virtual void GetMeshElements(FAssetHandle MeshHandle, Uint32 LODLevel, TArray<FMeshBatchElement>& OutElements) const = 0;

    /**
     * @brief 정적 메시 배치를 수집 구현에 전달합니다.
     * @param Mesh 제출할 메시·머티리얼·파이프라인과 LOD 구간입니다.
     * @note 구현은 호출 후 필요한 데이터를 보관할 책임이 있습니다.
     */
    virtual void DrawMesh(const FMeshBatch& Mesh) = 0;
};
