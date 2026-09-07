#include "PCH.h"
#include "AActor.h"
#include "Scene/UWorld.h"
#include "Component/USceneComponent.h"
#include "../Core/Base/TypeRegistry.h"

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

void AActor::Serialize(FArchive& Archive)
{
    UObject::Serialize(Archive);

    // components
    size_t ArraySize = Components.size();
    Archive.BeginArrayScope("Components", ArraySize);

    for (size_t i = 0; i < ArraySize; ++i)
    {
        Archive.BeginObjectScope(std::to_string(i));
        Components[i]->Serialize(Archive);
        Archive.EndObjectScope();
    }

    Archive.EndArrayScope();



    // root component
    FString GuidRootComponent;
    if (RootComponent != nullptr)
        GuidRootComponent = RootComponent->GetGuid().ToString();

    Archive.Serialize("GuidRootComponent", GuidRootComponent);

    if (Archive.IsLoading() && !GuidRootComponent.empty())
    {
        FGuid Guid;
        Guid.Parse(GuidRootComponent);
        RootComponent = static_cast<USceneComponent*>(UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(Guid)));
    }
}


void AActor::PreLoadComponents(FArchive& Archive)
{
    size_t ArraySize = 0;
    Archive.BeginArrayScope("Components", ArraySize);

    Components.clear();
    Components.resize(ArraySize);

    for (size_t i = 0; i < ArraySize; ++i)
    {
        Archive.BeginObjectScope(std::to_string(i));

        FString TypeName;
        Archive.Serialize("TypeName", TypeName);

        Components[i] = std::unique_ptr<UActorComponent>(
            static_cast<UActorComponent*>(TypeRegistry::Find(TypeName)->Creator().release())
        );
        Components[i]->SetOwner(this);

        FGuid ComponentGuid;
        Archive.Serialize("Guid", ComponentGuid);
        UObjectSystem::RegisterWithGuid(Components[i].get(), ComponentGuid);

        Archive.EndObjectScope();
    }

    Archive.EndArrayScope();
}