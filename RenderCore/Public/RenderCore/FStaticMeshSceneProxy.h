#pragma once

#include "RenderCore/FPrimitiveSceneProxy.h"

class FStaticMeshSceneProxy final : public FPrimitiveSceneProxy {
public:
    FStaticMeshSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform, const FMeshSceneData& MeshData);
    ~FStaticMeshSceneProxy() override = default;

public:
    const FMeshSceneData& GetMeshData() const override;

private:
    FMeshSceneData mMeshData{};
};
