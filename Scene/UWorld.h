#pragma once

#include <filesystem>
#include "Common.h"
#include "Core/Base/UObject.h"
#include "Core/Base/UObjectSystem.h"
#include "Core/Base/FRenderProbe.h"

class AActor;
class UCameraComponent;
class UStaticMeshComponent;
struct ID3D11Device;
class FAssetRegistry;

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
    FRenderProbe& BuildRenderProbe() ;
    void Tick(float DeltaTime);

    void RegisterRenderable(UStaticMeshComponent* Component);
    void UnregisterRenderable(UStaticMeshComponent* Component);
    void SetMainCamera(UCameraComponent* InCamera);
    void ClearMainCamera(UCameraComponent* InCamera);

    bool SaveScene(const FString& SceneName, FAssetRegistry* AssetRegistry);
    bool LoadScene(const std::filesystem::path& ScenePath, ID3D11Device* Device, FAssetRegistry* AssetRegistry);

	JG_DECLARE_DERIVED_TYPEINFO(UWorld, UObject);
private:
    std::vector<std::unique_ptr<AActor>> Actors;
    std::vector<UStaticMeshComponent*> RenderableComponents;
    UCameraComponent* Camera = nullptr;
    FRenderProbe Probe{};
};
