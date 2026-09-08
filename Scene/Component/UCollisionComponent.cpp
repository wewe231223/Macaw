#include "PCH.h"
#include <DirectXCollision.h>

#include "UCollisionComponent.h"
#include "UStaticMeshComponent.h"
#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "Core/Asset/UMesh.h"
#include "Core/Asset/FAssetRegistry.h"

#include "../../Core/Console/Console.h"


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

void UCollisionComponent::SetBounds(const DirectX::BoundingBox& InBounds) {
	DirectX::BoundingOrientedBox::CreateFromBoundingBox(OBB, InBounds);
}

bool UCollisionComponent::Raycast(const FRay& Ray, float& OutDistance) const
{
    if (!bCollisionEnabled)
        return false;

	auto cast = RaycastBounds(Ray, OutDistance);

    if (cast) {
		cast = RaycastMesh(Ray, *GetOwner()->GetComponent<UStaticMeshComponent>(), OutDistance);
    }

    return cast;
}

const FVector3 UCollisionComponent::GetExtent() const {
	return FVector3{ OBB.Extents.x, OBB.Extents.y, OBB.Extents.z };
}

void UCollisionComponent::SetExtent(const FVector3& InExtent)
{
	OBB.Extents = DirectX::XMFLOAT3(InExtent.x, InExtent.y, InExtent.z);
}

bool UCollisionComponent::RaycastBounds(const FRay& Ray, float& OutDistance) const {
    DirectX::BoundingOrientedBox WorldBox;
    OBB.Transform(WorldBox, GetWorldMatrix());

    return WorldBox.Intersects(Ray.position, Ray.direction, OutDistance);
}

bool UCollisionComponent::RaycastMesh(const FRay& Ray, const UStaticMeshComponent& MeshComponent, float& OutDistance) const {
    AActor* Owner = MeshComponent.GetOwner();
    if (Owner == nullptr || Owner->GetWorld() == nullptr)
        return false;

    FAssetRegistry* Registry = Owner->GetWorld()->GetAssetRegistry();
    if (Registry == nullptr)
        return false;

    UMesh* Mesh = Registry->ResolveAsset<UMesh>(
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

    const FMatrix WorldMatrix = GetWorldMatrix(); 

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

void UCollisionComponent::MakeRender(FActorProbe& Probe) const
{
}

void UCollisionComponent::Serialize(FArchive& Archive) 
{
    UPrimitiveComponent::Serialize(Archive);

    FGuid GuidParent{};
    if (Archive.IsSaving()) {
        GuidParent = GetParent()->GetGuid(); 
    }


    Archive.Serialize("Parent", GuidParent);
    Archive.Serialize("OBB_Center", static_cast<FVector3&>(OBB.Center));
    Archive.Serialize("OBB_Extent", static_cast<FVector3&>(OBB.Extents));
    Archive.Serialize("OBB_Orientation", static_cast<FQuat&>(OBB.Orientation));
    Archive.Serialize("bCollisionEnabled", bCollisionEnabled);

    if (Archive.IsLoading() && GuidParent.IsValid())
    {
        FGuid Guid;
        Guid.Parse(GuidParent.ToString());
		UObjectSystem::FindHandleByGuid(Guid);
        AttachTo(static_cast<USceneComponent*>(UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(Guid)))); 
    }
}