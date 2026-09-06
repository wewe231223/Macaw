#pragma once

#include "UPrimitiveComponent.h"
#include "Core/Asset/FAssetHandle.h"

class UStaticMeshComponent : public UPrimitiveComponent
{
public:
    UStaticMeshComponent() = default;
    ~UStaticMeshComponent() override = default;

    FAssetHandle GetMeshHandle() const;
    FAssetHandle GetMaterialHandle() const;
    FAssetHandle GetPipelineHandle() const;

    void SetMeshHandle(FAssetHandle InHandle);
    void SetMaterialHandle(FAssetHandle InHandle);
    void SetPipelineHandle(FAssetHandle InHandle);

private:
    FAssetHandle MeshHandle;
    FAssetHandle MaterialHandle;
    FAssetHandle PipelineHandle;
};