#pragma once

#include "Common.h"
#include "Core/Base/UObject.h"
#include "Core/Base/UObjectSystem.h"
#include "Core/Base/FRenderProbe.h"
#include "Component/UCameraComponent.h"
#include "Component/UStaticMeshComponent.h"

class UWorld : public UObject
{
public:
    UWorld() = default;
    ~UWorld() override = default;

    template<typename T>
     requires std::is_base_of_v<AActor, T>
    T* SpawnActor()
    {
        std::unique_ptr<T> NewActor = std::make_unique<T>();

        T* ActorPtr = NewActor.get();

        UObjectSystem::Register(ActorPtr);

        ActorPtr->SetWorld(this);
        Actors.push_back(std::move(NewActor));

        return ActorPtr;
    }

    const std::vector<std::unique_ptr<AActor>>& GetObjects() const;
    FRenderProbe BuildRenderProbe() const;
    void RegisterComponent(UActorComponent* Component);

private:
    std::vector<std::unique_ptr<AActor>> Actors;
    std::vector<UStaticMeshComponent*> StaticMeshComponents;

    UCameraComponent* Camera = nullptr;
    
};