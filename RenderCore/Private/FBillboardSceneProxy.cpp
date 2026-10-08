#include "pch.h"
#include "RenderCore/FBillboardSceneProxy.h"
#include "RenderCore/FDynamicPrimitiveDrawInterface.h"

FBillboardSceneProxy::FBillboardSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform, FBillboardDrawData Data)
	: FPrimitiveSceneProxy(ComponentHandle, OwnerHandle, Transform),
	  mData(std::move(Data)) {
}

void FBillboardSceneProxy::GetDynamicMeshElements(FDynamicPrimitiveDrawInterface& DrawInterface) const {
    DrawInterface.DrawBillboard(mData, GetTransform().mWorld);
}
