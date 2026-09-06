#include "PCH.h"
#include "UActorComponent.h"

AActor* UActorComponent::GetOwner() const
{
    return Owner;
}

void UActorComponent::SetOwner(AActor* InOwner)
{
    Owner = InOwner;
}

bool UActorComponent::IsActive() const
{
    return bActive;
}

void UActorComponent::SetActive(bool bInActive)
{
    bActive = bInActive;
}