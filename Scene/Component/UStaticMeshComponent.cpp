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


void UStaticMeshComponent::Serialize(FArchive& Archive)
{
    UPrimitiveComponent::Serialize(Archive);

    FString GuidMeshHandle;
    if (MeshHandle.ID != std::numeric_limits<uint32>::max())
        ; // todo this function is not updated yet
        // GuidMeshHandle = Archive.GetAssetRegistry()->ResolveAsset<UObject>(MeshHandle)->GetGuid().ToString();
    Archive.Serialize("GuidMeshHandle", GuidMeshHandle);
    if (Archive.IsLoading())
    {
        FGuid Guid;
        Guid.Parse(GuidMeshHandle);

        // TODO function is not updated yet
        // MeshHandle = Archive.GetAssetRegistry()->GetAsset(Guid);
    }

    FString GuidMaterialHandle;
    if (MaterialHandle.ID != std::numeric_limits<uint32>::max())
        ; // todo this function is not updated yet
        // GuidMaterialHandle = Archive.GetAssetRegistry()->ResolveAsset<UObject>(MaterialHandle)->GetGuid().ToString();
    Archive.Serialize("GuidMaterialHandle", GuidMaterialHandle);
    if (Archive.IsLoading())
    {
        FGuid Guid;
        Guid.Parse(GuidMaterialHandle);

        // TODO function is not updated yet
        // MaterialHandle = Archive.GetAssetRegistry()->GetAsset(Guid);
    }

    FString GuidPipelineHandle;
    if (PipelineHandle.ID != std::numeric_limits<uint32>::max())
        ; // todo this function is not updated yet
        // GuidPipelineHandle = Archive.GetAssetRegistry()->ResolveAsset<UObject>(PipelineHandle)->GetGuid().ToString();
    Archive.Serialize("GuidPipelineHandle", GuidPipelineHandle);
    if (Archive.IsLoading())
    {
        FGuid Guid;
        Guid.Parse(GuidPipelineHandle);

        // TODO function is not updated yet
        // PipelineHandle = Archive.GetAssetRegistry()->GetAsset(Guid);
    }
}