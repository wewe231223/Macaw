#pragma once

#include "Common.h"
#include "Core/Base/UObject.h"
#include "Core/Base/UObjectSystem.h"
#include "Core/Base/FRenderProbe.h"
class AActor;
class UCameraComponent;
class UPrimitiveComponent;

class UWorld : public UObject
{
public:
    UWorld() = default;
    ~UWorld() override;

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

    const std::vector<std::unique_ptr<AActor>>& GetActors() const;
    FRenderProbe BuildRenderProbe() const;
    void Tick(float DeltaTime);

    void RegisterRenderable(UPrimitiveComponent* Component);
    void UnregisterRenderable(UPrimitiveComponent* Component);
    void SetMainCamera(UCameraComponent* InCamera);
    void ClearMainCamera(UCameraComponent* InCamera);

private:
    std::vector<std::unique_ptr<AActor>> Actors;
    std::vector<UPrimitiveComponent*> RenderableComponents;
    UCameraComponent* Camera = nullptr;
};
