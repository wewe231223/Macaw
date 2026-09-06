#include "PCH.h"
#include "AActor.h"
#include "Scene/UWorld.h"
#include "Component/USceneComponent.h"

const std::vector<std::unique_ptr<UActorComponent>>&
AActor::GetComponents() const
{
    return Components;
}

USceneComponent* AActor::GetRootComponent()
{
    return RootComponent;
}

const USceneComponent* AActor::GetRootComponent() const
{
    return RootComponent;
}

void AActor::SetRootComponent(USceneComponent* InRootComponent)
{
    RootComponent = InRootComponent;
}

void AActor::SetWorld(UWorld* InWorld)
{
    World = InWorld;
}