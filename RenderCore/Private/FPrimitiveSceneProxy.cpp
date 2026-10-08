#include "pch.h"
#include "RenderCore/FPrimitiveSceneProxy.h"
#include "RenderCore/FRenderProbe.h"

FPrimitiveSceneProxy::FPrimitiveSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform)
	: mComponentHandle(ComponentHandle),
	  mOwnerHandle(OwnerHandle),
	  mTransform(Transform) {
}

FPrimitiveSceneProxy::~FPrimitiveSceneProxy() = default;

FObjectHandle FPrimitiveSceneProxy::GetComponentHandle() const {
    return mComponentHandle;
}

FObjectHandle FPrimitiveSceneProxy::GetOwnerHandle() const {
    return mOwnerHandle;
}

const FPrimitiveTransform& FPrimitiveSceneProxy::GetTransform() const {
    return mTransform;
}

void FPrimitiveSceneProxy::SetTransform(const FPrimitiveTransform& Transform) {
    mTransform = Transform;
}

void FPrimitiveSceneProxy::BuildLegacyProbe(FActorProbe& Probe) const {
    const FMeshSceneData& Mesh{GetMeshData()};

    Probe = FActorProbe{mTransform.mWorld, Mesh.mMeshHandle, Mesh.mMaterialHandle, Mesh.mPipelineHandle, mOwnerHandle, mTransform.mWorldSphereBounds, mTransform.mWorldOBB, mTransform.mWorldAABB};
}
