#pragma once

#include "Common.h"

#include "Core/Base/UObject.h"
#include "Scene/UWorld.h"
#include "Scene/Component/UActorComponent.h"
#include "Scene/Component/USceneComponent.h"

class AActor : public UObject
{
public:
    AActor() = default;
    ~AActor() override = default;

    template<typename T>
    requires std::is_base_of_v<UActorComponent, T>
    T* AddComponent()
    {
        std::unique_ptr<T> NewComponent = std::make_unique<T>();
        T* ComponentPtr = NewComponent.get();

        ComponentPtr->SetOwner(this);
        UObjectSystem::Register(ComponentPtr);

        Components.push_back(std::move(NewComponent));

        World->RegisterComponent(ComponentPtr);

        return ComponentPtr;
    }

    const std::vector<std::unique_ptr<UActorComponent>>& GetComponents() const;

    USceneComponent* GetRootComponent();
    const USceneComponent* GetRootComponent() const;

    void SetWorld(UWorld* InWorld);
    void SetRootComponent(USceneComponent* InRootComponent);

private:
    std::vector<std::unique_ptr<UActorComponent>> Components;
    USceneComponent* RootComponent = nullptr;

    UWorld* World = nullptr;
};