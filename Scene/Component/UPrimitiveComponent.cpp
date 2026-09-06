#include "PCH.h"
#include "UPrimitiveComponent.h"

#include "Scene/AActor.h"
#include "Scene/UWorld.h"

bool UPrimitiveComponent::IsVisible() const
{
    return bVisible;
}

void UPrimitiveComponent::SetVisible(bool bInVisible)
{
    bVisible = bInVisible;
}

void UPrimitiveComponent::OnCreate()
{
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr)
    {
        Owner->GetWorld()->RegisterRenderable(this);
    }
}

void UPrimitiveComponent::OnDestroy()
{
    AActor* Owner = GetOwner();

    if (Owner != nullptr && Owner->GetWorld() != nullptr)
    {
        Owner->GetWorld()->UnregisterRenderable(this);
    }
}
