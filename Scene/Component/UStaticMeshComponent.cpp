#include "PCH.h"
#include "UStaticMeshComponent.h"

#include "Core/Base/FRenderProbe.h"

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

void UStaticMeshComponent::MakeRender(FRenderProbe& OutProbe) const
{
    if (!IsActive() || !IsVisible())
    {
        return;
    }

    OutProbe.ActorProbes.push_back({
        GetTransform().GetWorldMatrix(),
        MeshHandle,
        MaterialHandle,
        PipelineHandle
    });
}
