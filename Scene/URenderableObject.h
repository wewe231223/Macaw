#pragma once

#include "USceneObject.h"
#include "Core/Asset/FAssetHandle.h"

class URenderableObject : public USceneObject
{
public:
    URenderableObject() = default;
    ~URenderableObject() override = default;

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