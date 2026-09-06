#include "PCH.h"
#include "UStaticMeshComponent.h"

#include "Core/Base/FRenderProbe.h"
#include "Scene/AActor.h"
#include "Scene/UWorld.h"

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

void UStaticMeshComponent::OnCreate()
{
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr)
    {
        Owner->GetWorld()->RegisterRenderable(this);
    }
}

void UStaticMeshComponent::OnDestroy()
{
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr)
    {
        Owner->GetWorld()->UnregisterRenderable(this);
    }
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
