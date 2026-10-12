#pragma once
#include "RenderCore/FPrimitiveSceneProxy.h"
#include "RenderCore/FRenderData.h"

class FBillboardSceneProxy final : public FPrimitiveSceneProxy {
public:
    FBillboardSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform, FBillboardDrawData Data);
    ~FBillboardSceneProxy() override = default;

public:
    void GetDynamicMeshElements(FDynamicPrimitiveDrawInterface& DrawInterface) const override;
    void SetBillboardUV(const FVector2& UVMin, const FVector2& UVMax) override;

private:
    FBillboardDrawData mData{};
};
