#include "PCH.h"
#include "UStaticMeshComponent.h"

FAssetHandle UStaticMeshComponent::GetMeshHandle() const { return MeshHandle; }

FAssetHandle UStaticMeshComponent::GetMaterialHandle() const { return MaterialHandle; }

FAssetHandle UStaticMeshComponent::GetPipelineHandle() const { return PipelineHandle; }

void UStaticMeshComponent::SetMeshHandle(FAssetHandle InHandle)
{
    MeshHandle = InHandle;
}

void UStaticMeshComponent::SetMaterialHandle(FAssetHandle InHandle)
{
    MaterialHandle = InHandle;
}

void UStaticMeshComponent::SetPipelineHandle(FAssetHandle InHandle)
{
    PipelineHandle = InHandle;
}