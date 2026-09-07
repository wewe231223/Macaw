#include "PCH.h"
#include <DirectXCollision.h>

#include "UCollisionComponent.h"
#include "Scene/AActor.h"
#include "Scene/UWorld.h"

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
}

void UCollisionComponent::OnDestroy()
{
    AActor* Owner = GetOwner();

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
    {
        return false;
    }

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
    // 선택 사항: 디버그 모드일 때만 Probe에 와이어프레임 박스 렌더링 요청 추가
}

void UCollisionComponent::Serialize(FArchive& Archive)
{
    UPrimitiveComponent::Serialize(Archive);
    Archive.Serialize("Extent", Extent);
    Archive.Serialize("bCollisionEnabled", bCollisionEnabled);
}