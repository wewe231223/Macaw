#pragma once

#include "CoreUObject/UObject.h"

class AActor;
class UWorld;

class UActorComponent : public UObject {
public:
    ReflectTypes(UActorComponent, UObject);

public:
    UActorComponent();
    ~UActorComponent() override;

public:
    ReflectClass(UActorComponent);

    AActor* GetOwner() const;
    UWorld* GetWorld() const;
    void SetOwner(AActor* Owner);

    virtual void RegisterComponent(UWorld& World);
    virtual void UnregisterComponent();
    bool IsRegistered() const;

private:
    ReflectMembers();

private:
    AActor* mOwner{};
    UWorld* mWorld{};
};
