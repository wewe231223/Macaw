#pragma once

#include "Common.h"

#include "Core/Base/UObject.h"
#include "Scene/Component/UActorComponent.h"
#include "Scene/Component/USceneComponent.h"

class UWorld;

class AActor : public UObject
{
public:
    AActor() = default;
    ~AActor() override;

    template<typename T>
    requires std::is_base_of_v<UActorComponent, T>
    T* AddComponent()
    {
        std::unique_ptr<T> NewComponent = std::make_unique<T>();
        T* ComponentPtr = NewComponent.get();

        ComponentPtr->SetOwner(this);
        UObjectSystem::Register(ComponentPtr);

        Components.push_back(std::move(NewComponent));

        if (World != nullptr)
        {
            ComponentPtr->OnCreate();
        }

        return ComponentPtr;
    }

    const TArray<std::unique_ptr<UActorComponent>>& GetComponents() const;

    USceneComponent* GetRootComponent();
    const USceneComponent* GetRootComponent() const;

    void SetWorld(UWorld* InWorld);
    UWorld* GetWorld() const;
    void SetRootComponent(USceneComponent* InRootComponent);
    void Tick(float DeltaTime);

    void PreLoadComponents(FArchive& Archive);

	JG_DECLARE_DERIVED_TYPEINFO(AActor, UObject);
protected:
    void Serialize(FArchive& Archive) override;

private:
    TArray<std::unique_ptr<UActorComponent>> Components{};
    USceneComponent* RootComponent = nullptr;

    UWorld* World = nullptr;
};
