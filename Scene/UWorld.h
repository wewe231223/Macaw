#pragma once

#include "Common.h"
#include "Core/Base/UObject.h"
#include "Core/Base/UObjectSystem.h"
#include "Core/Base/FRenderProbe.h"
class AActor;
class UCameraComponent;
class UStaticMeshComponent;

class UWorld : public UObject
{
public:
    UWorld() = default;
    ~UWorld() override;

    AActor* AddActor(std::unique_ptr<AActor> InActor);

    template<typename T>
    requires std::is_base_of_v<AActor, T>
    T* SpawnActor()
    {
        std::unique_ptr<T> NewActor = std::make_unique<T>();

        T* ActorPtr = NewActor.get();

        if (AddActor(std::move(NewActor)) == nullptr)
        {
            return nullptr;
        }

        return ActorPtr;
    }

    const std::vector<std::unique_ptr<AActor>>& GetActors() const;
    FRenderProbe& BuildRenderProbe() ;
    void Tick(float DeltaTime);

    void RegisterRenderable(UStaticMeshComponent* Component);
    void UnregisterRenderable(UStaticMeshComponent* Component);
    void SetMainCamera(UCameraComponent* InCamera);
    void ClearMainCamera(UCameraComponent* InCamera);


private:
    std::vector<std::unique_ptr<AActor>> Actors;
    std::vector<UStaticMeshComponent*> RenderableComponents;
    std::vector<UStaticMeshComponent*> CollisionComponents;
 
    UCameraComponent* Camera = nullptr;
    FRenderProbe Probe{};
};
