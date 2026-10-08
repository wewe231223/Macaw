#include "pch.h"
#include "RenderCore/FStaticMeshSceneProxy.h"
#include "RenderCore/FStaticPrimitiveDrawInterface.h"

FStaticMeshSceneProxy::FStaticMeshSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform, const FMeshSceneData& MeshData)
	: FPrimitiveSceneProxy(ComponentHandle, OwnerHandle, Transform),
	  mMeshData(MeshData) {
}

const FMeshSceneData& FStaticMeshSceneProxy::GetMeshData() const {
    return mMeshData;
}

void FStaticMeshSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface& DrawInterface) const {
    const Uint32 LODCount{DrawInterface.GetMeshLODCount(mMeshData.mMeshHandle)};

    for (Uint32 Level{}; Level < LODCount; ++Level) {
        FMeshBatch Batch{};

        Batch.mMeshHandle = mMeshData.mMeshHandle;
        Batch.mMaterialHandle = mMeshData.mMaterialHandle;
        Batch.mPipelineHandle = mMeshData.mPipelineHandle;
        Batch.mLODLevel = Level;
        DrawInterface.GetMeshElements(Batch.mMeshHandle, Level, Batch.mElements);

        if (!Batch.mElements.empty()) {
            DrawInterface.DrawMesh(Batch);
        }
    }
}
