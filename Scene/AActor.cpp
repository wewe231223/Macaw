#include "PCH.h"
#include "AActor.h"
#include "Scene/UWorld.h"
#include "Component/USceneComponent.h"

const std::vector<std::unique_ptr<UActorComponent>>&
AActor::GetComponents() const
{
    return Components;
}

AActor::~AActor()
{
    for (const std::unique_ptr<UActorComponent>& Component : Components)
    {
        Component->OnDestroy();
        UObjectSystem::Unregister(Component.get(), Component->GetHandle());
    }
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

    if (World == nullptr)
    {
        return;
    }

    for (const std::unique_ptr<UActorComponent>& Component : Components)
    {
        Component->OnCreate();
    }
}

UWorld* AActor::GetWorld() const
{
    return World;
}

void AActor::Tick(float DeltaTime)
{
    for (const std::unique_ptr<UActorComponent>& Component : Components)
    {
        if (Component->IsActive())
        {
            Component->Tick(DeltaTime);
        }
    }
}
