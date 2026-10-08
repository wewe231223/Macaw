#pragma once
#include "RenderCore/FMeshBatch.h"

class FStaticPrimitiveDrawInterface {
public:
    virtual ~FStaticPrimitiveDrawInterface();

public:
    virtual Uint32 GetMeshLODCount(FAssetHandle MeshHandle) const = 0;
    virtual void GetMeshElements(FAssetHandle MeshHandle, Uint32 LODLevel, TArray<FMeshBatchElement>& OutElements) const = 0;
    virtual void DrawMesh(const FMeshBatch& Mesh) = 0;
};
