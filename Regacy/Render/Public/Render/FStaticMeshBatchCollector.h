/**
 * @file FStaticMeshBatchCollector.h
 * @brief 전역 메시 에셋을 조회하는 정적 메시 배치 수집기를 정의합니다.
 */

#pragma once
#include "RenderCore/FStaticPrimitiveDrawInterface.h"

class FStaticMeshBatchCollector final : public FStaticPrimitiveDrawInterface {
public:
    /**
     * @brief 전역 메시 에셋을 조회해 지정한 배열에 배치를 수집합니다.
     * @param Meshes 수집한 배치를 추가할 배열입니다.
     */
    explicit FStaticMeshBatchCollector(TArray<FMeshBatch>& Meshes);
    ~FStaticMeshBatchCollector() override = default;

public:
    Uint32 GetMeshLODCount(FAssetHandle MeshHandle) const override;
    void GetMeshElements(FAssetHandle MeshHandle, Uint32 LODLevel, TArray<FMeshBatchElement>& OutElements) const override;
    void DrawMesh(const FMeshBatch& Mesh) override;

private:
    TArray<FMeshBatch>& mMeshes;
};
