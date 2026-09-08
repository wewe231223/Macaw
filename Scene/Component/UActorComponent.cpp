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

void UActorComponent::OnCreate()
{
}

void UActorComponent::Tick(float /*DeltaTime*/)
{
}

void UActorComponent::OnDestroy()
{
    bActive = false;
    Owner = nullptr;
}

bool UActorComponent::IsActive() const
{
    return bActive;
}

void UActorComponent::SetActive(bool bInActive)
{
    bActive = bInActive;
}

void UActorComponent::Serialize(FArchive& Archive)
{
    UObject::Serialize(Archive);

    Archive.Serialize("bActive", bActive);
}