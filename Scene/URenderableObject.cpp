#include "PCH.h"
#include "URenderableObject.h"

FAssetHandle URenderableObject::GetMeshHandle() const { return MeshHandle; }

FAssetHandle URenderableObject::GetMaterialHandle() const{ return MaterialHandle; }

FAssetHandle URenderableObject::GetPipelineHandle() const { return PipelineHandle;}

void URenderableObject::SetMeshHandle(FAssetHandle InHandle)
{
    MeshHandle = InHandle;
}

void URenderableObject::SetMaterialHandle(FAssetHandle InHandle)
{
    MaterialHandle = InHandle;
}

void URenderableObject::SetPipelineHandle(FAssetHandle InHandle)
{
    PipelineHandle = InHandle;
}