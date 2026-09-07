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
    if (!bCollisionEnabled)
        return false;

    float BoundsDistance = 0.0f;

    if (!RaycastBounds(Ray, BoundsDistance))
        return false;

    AActor* Owner = GetOwner();
    if (Owner == nullptr)
    {
        OutDistance = BoundsDistance;
        return true;
    }

    UStaticMeshComponent* MeshComponent = Owner->GetComponent<UStaticMeshComponent>();
    if (MeshComponent == nullptr)
    {
        OutDistance = BoundsDistance;
        return true;
    }

    return RaycastMesh(Ray, *MeshComponent, OutDistance);
}

bool UCollisionComponent::RaycastBounds(const FRay& Ray, float& OutDistance) const
{
    DirectX::BoundingOrientedBox LocalBox;

    LocalBox.Center = LocalCenter;
    LocalBox.Extents = Extent;
    LocalBox.Orientation = FQuat(0.f, 0.f, 0.f, 1.f);

    DirectX::BoundingOrientedBox WorldBox;
    LocalBox.Transform(WorldBox, GetWorldMatrix());

    return WorldBox.Intersects(Ray.position, Ray.direction, OutDistance);
}

bool UCollisionComponent::RaycastMesh(
    const FRay& Ray,
    const UStaticMeshComponent& MeshComponent,
    float& OutDistance) const
{
    AActor* Owner = MeshComponent.GetOwner();
    if (Owner == nullptr || Owner->GetWorld() == nullptr)
        return false;

    FAssetRegistry* Registry = Owner->GetWorld()->GetAssetRegistry();
    if (Registry == nullptr)
        return false;

    UMesh* Mesh = Registry->ResolveAsset<UMesh>(
        EAssetType::Mesh,
        MeshComponent.GetMeshHandle());

    if (Mesh == nullptr)
    {
        return false;
    }

    const auto Positions = Mesh->GetVertexAttributeData<EVertexAttribute::Position>();

    const auto& Indices = Mesh->GetIndices();

    if (Positions.empty() || Indices.size() < 3)
    {
        return false;
    }

    const FMatrix WorldMatrix = MeshComponent.GetWorldMatrix();

    bool bHit = false;
    float ClosestDistance = std::numeric_limits<float>::max();

    for (size_t i = 0; i + 2 < Indices.size(); i += 3)
    {
        const uint32 I0 = Indices[i];
        const uint32 I1 = Indices[i + 1];
        const uint32 I2 = Indices[i + 2];

        if (I0 >= Positions.size() ||
            I1 >= Positions.size() ||
            I2 >= Positions.size())
        {
            continue;
        }

        const DirectX::XMVECTOR V0 =
            DirectX::XMVector3TransformCoord(
                DirectX::XMLoadFloat3(&Positions[I0]),
                WorldMatrix);

        const DirectX::XMVECTOR V1 =
            DirectX::XMVector3TransformCoord(
                DirectX::XMLoadFloat3(&Positions[I1]),
                WorldMatrix);

        const DirectX::XMVECTOR V2 =
            DirectX::XMVector3TransformCoord(
                DirectX::XMLoadFloat3(&Positions[I2]),
                WorldMatrix);

        float Distance = 0.0f;

        if (DirectX::TriangleTests::Intersects(
            Ray.position,
            Ray.direction,
            V0,
            V1,
            V2,
            Distance))
        {
            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                bHit = true;
            }
        }
    }

    if (bHit)
    {
        OutDistance = ClosestDistance;
    }

    return bHit;
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