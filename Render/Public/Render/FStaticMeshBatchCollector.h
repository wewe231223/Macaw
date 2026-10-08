#pragma once
#include "RenderCore/FStaticPrimitiveDrawInterface.h"
#include "Render/FStaticMeshBatch.h"

class IAssetRegistry;

class FStaticMeshBatchCollector final : public FStaticPrimitiveDrawInterface {
public:
    FStaticMeshBatchCollector(const IAssetRegistry& Registry, Uint32 ObjectIndex, TArray<FStaticMeshBatch>& Meshes);
    ~FStaticMeshBatchCollector() override = default;

public:
    Uint32 GetMeshLODCount(FAssetHandle MeshHandle) const override;
    void GetMeshElements(FAssetHandle MeshHandle, Uint32 LODLevel, TArray<FMeshBatchElement>& OutElements) const override;
    void DrawMesh(const FMeshBatch& Mesh) override;

private:
    const IAssetRegistry& mRegistry;
    Uint32 mObjectIndex{};
    TArray<FStaticMeshBatch>& mMeshes;
};
