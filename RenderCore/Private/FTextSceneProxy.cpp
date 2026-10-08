#include "pch.h"
#include "RenderCore/FTextSceneProxy.h"
#include "RenderCore/FDynamicPrimitiveDrawInterface.h"

FTextSceneProxy::FTextSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform, FTextDrawData Data)
	: FPrimitiveSceneProxy(ComponentHandle, OwnerHandle, Transform),
	  mData(std::move(Data)) {
}

void FTextSceneProxy::GetDynamicMeshElements(FDynamicPrimitiveDrawInterface& DrawInterface) const {
    DrawInterface.DrawText(mData, GetTransform().mWorld);
}
