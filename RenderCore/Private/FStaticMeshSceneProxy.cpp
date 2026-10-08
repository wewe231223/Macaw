#include "pch.h"
#include "RenderCore/FStaticMeshSceneProxy.h"

FStaticMeshSceneProxy::FStaticMeshSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform, const FMeshSceneData& MeshData)
	: FPrimitiveSceneProxy(ComponentHandle, OwnerHandle, Transform),
	  mMeshData(MeshData) {
}

const FMeshSceneData& FStaticMeshSceneProxy::GetMeshData() const {
    return mMeshData;
}
