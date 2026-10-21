#pragma once

#include "CoreUObject/UObject.h"

class UWorld;
class UActorComponent;
class USceneComponent;
struct FTransform;

class AActor : public UObject {
public:
    ReflectTypes(AActor, UObject);

public:
    AActor();
    ~AActor() override;

public:
    ReflectClass(AActor);

    UWorld* GetWorld() const;
    void SetWorld(UWorld* World);

    bool AddComponent(std::unique_ptr<UActorComponent> Component);
    const TArray<std::unique_ptr<UActorComponent>>& GetComponents() const;

    USceneComponent* GetRootComponent() const;
    bool SetActorTransform(const FTransform& Transform);

    virtual bool InitializeDefaultComponents();

private:
    ReflectMembers();

private:
    UWorld* mWorld{};

    TArray<std::unique_ptr<UActorComponent>> mComponents{};
    USceneComponent* mRootComponent{};
};
