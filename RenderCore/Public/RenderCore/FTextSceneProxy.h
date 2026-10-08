#pragma once
#include "RenderCore/FPrimitiveSceneProxy.h"
#include "RenderCore/FRenderData.h"

class FTextSceneProxy final : public FPrimitiveSceneProxy {
public:
    FTextSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform, FTextDrawData Data);
    ~FTextSceneProxy() override = default;

public:
    void GetDynamicMeshElements(FDynamicPrimitiveDrawInterface& DrawInterface) const override;

private:
    FTextDrawData mData{};
};
