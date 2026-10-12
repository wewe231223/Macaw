#include "pch.h"
#include "RenderCore/FBillboardSceneProxy.h"
#include "RenderCore/FDynamicPrimitiveDrawInterface.h"

FBillboardSceneProxy::FBillboardSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform, FBillboardDrawData Data)
	: FPrimitiveSceneProxy(ComponentHandle, OwnerHandle, Transform),
	  mData(std::move(Data)) {
}

void FBillboardSceneProxy::GetDynamicMeshElements(FDynamicPrimitiveDrawInterface& DrawInterface) const {
    FBillboardDrawData Data{mData};
    const FUVTransform UV{GetUVTransform()};

    Data.mUvMin = {mData.mUvMin.mX * UV.mScale.mX + UV.mOffset.mX, mData.mUvMin.mY * UV.mScale.mY + UV.mOffset.mY};
    Data.mUvMax = {mData.mUvMax.mX * UV.mScale.mX + UV.mOffset.mX, mData.mUvMax.mY * UV.mScale.mY + UV.mOffset.mY};
    DrawInterface.DrawBillboard(Data, GetTransform().mWorld);
}

void FBillboardSceneProxy::SetBillboardUV(const FVector2& UVMin, const FVector2& UVMax) {
    mData.mUvMin = UVMin;
    mData.mUvMax = UVMax;
}
