#include "pch.h"
#include "RenderCore/FPrimitiveSceneProxy.h"

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

void FPrimitiveSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface& DrawInterface) const {
}

void FPrimitiveSceneProxy::SetTransform(const FPrimitiveTransform& Transform) {
    mTransform = Transform;
}

const FMeshSceneData* FPrimitiveSceneProxy::GetMeshData() const {
    return nullptr;
}

void FPrimitiveSceneProxy::GetDynamicMeshElements(FDynamicPrimitiveDrawInterface& DrawInterface) const {
}
