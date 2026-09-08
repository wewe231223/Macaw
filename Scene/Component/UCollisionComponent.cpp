#include "PCH.h"
#include <DirectXCollision.h>

#include "UCollisionComponent.h"
#include "UStaticMeshComponent.h"
#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "Core/Asset/UMesh.h"
#include "Core/Asset/FAssetRegistry.h"


bool UCollisionComponent::IsCollisionEnabled() const
{
    return bCollisionEnabled;
}

void UCollisionComponent::SetCollisionEnabled(bool bEnabled)
{
    bCollisionEnabled = bEnabled;
}

void UCollisionComponent::OnCreate()
{
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr)
    {
        Owner->GetWorld()->RegisterCollision(this);
    }
}

void UCollisionComponent::OnDestroy()
{
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr)
    {
        Owner->GetWorld()->UnregisterCollision(this);
    }

    UPrimitiveComponent::OnDestroy();
}

void UCollisionComponent::SetBounds(const FVector3& InCenter, const FVector3& InExtent)
{
    LocalCenter = InCenter;
    Extent = InExtent;
}

bool UCollisionComponent::Raycast(const FRay& Ray, float& OutDistance) const
{
    DirectX::BoundingOrientedBox LocalBox;

    LocalBox.Center = LocalCenter;
    LocalBox.Extents = Extent;
    LocalBox.Orientation = FQuat(0.0f, 0.0f, 0.0f, 1.0f);

    FMatrix WorldMatrix = GetWorldMatrix();

    DirectX::BoundingOrientedBox WorldBox;
    LocalBox.Transform(WorldBox, WorldMatrix);

    return WorldBox.Intersects(Ray.position, Ray.direction, OutDistance);
}

const FVector3& UCollisionComponent::GetExtent() const
{
    return Extent;
}

void UCollisionComponent::SetExtent(const FVector3& InExtent)
{
    Extent = InExtent;
}

void UCollisionComponent::MakeRender(FRenderProbe& Probe) const
{
}

void UCollisionComponent::Serialize(FArchive& Archive)
{
    UPrimitiveComponent::Serialize(Archive);
    Archive.Serialize("Extent", Extent);
    Archive.Serialize("bCollisionEnabled", bCollisionEnabled);
}