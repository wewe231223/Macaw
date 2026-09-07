#include "PCH.h"
#include "UCameraComponent.h"

#include "Scene/AActor.h"
#include "Scene/UWorld.h"

FMatrix UCameraComponent::GetViewMatrix() const
{
    return GetTransform().GetWorldMatrix().Invert();
}

FMatrix UCameraComponent::GetProjectionMatrix() const
{
    return FMatrix::CreatePerspectiveFieldOfView(
        FOV,
        AspectRatio,
        NearPlane,
        FarPlane
    );
}

FMatrix UCameraComponent::GetViewProjectionMatrix() const
{
    return GetViewMatrix() * GetProjectionMatrix();
}

void UCameraComponent::OnCreate()
{
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr)
    {
        Owner->GetWorld()->SetMainCamera(this);
    }
}

void UCameraComponent::OnDestroy()
{
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr)
    {
        Owner->GetWorld()->ClearMainCamera(this);
    }
}


void UCameraComponent::Serialize(FArchive& Archive)
{
    USceneComponent::Serialize(Archive);

    Archive.Serialize("FOV", FOV);
    Archive.Serialize("AspectRatio", AspectRatio);
    Archive.Serialize("NearPlane", NearPlane);
    Archive.Serialize("FarPlane", FarPlane);
}