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
}

bool UActorComponent::IsTickEnabled() const
{
    return bTickEnabled;
}

void UActorComponent::SetTickEnabled(bool bInTickEnabled)
{
    bTickEnabled = bInTickEnabled;
}
