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
}

bool UCollisionComponent::Raycast(const FRay& Ray, float& OutDistance) const
{
    if (!bCollisionEnabled)
    {
        return false;
    }

    const FTransform& Transform = GetTransform();

    const FVector3& Position = Transform.GetPosition();
    const FRotator& Rotation = Transform.GetRotation();
    const FVector3& Scale = Transform.GetScale();
    
    DirectX::BoundingOrientedBox Box;

    Box.Center = Position;

    Box.Extents =
    {
        Extent.x * std::abs(Scale.x),
        Extent.y * std::abs(Scale.y),
        Extent.z * std::abs(Scale.z)
    };

    FQuat Orientation =
        FQuat::CreateFromYawPitchRoll(
            Rotation.y,
            Rotation.x,
            Rotation.z
        );

    Box.Orientation = Orientation;

    return Box.Intersects(Ray.position, Ray.direction, OutDistance);
}

const FVector3& UCollisionComponent::GetExtent() const
{
    return Extent;
}

void UCollisionComponent::SetExtent(const FVector3& InExtent)
{
    Extent = InExtent;
}

