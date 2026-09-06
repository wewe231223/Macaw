#pragma once

#include "Core/Base/UObject.h"

class AActor;

class UActorComponent : public UObject
{
public:
    UActorComponent() = default;
    ~UActorComponent() override = default;

    AActor* GetOwner() const;

    // Called after the owner and initial settings are ready.
    virtual void OnCreate();
    virtual void Tick(float DeltaTime);
    // Called before deletion, while the owner is still alive.
    virtual void OnDestroy();

    bool IsTickEnabled() const;
    void SetTickEnabled(bool bInTickEnabled);

private:
    friend class AActor;

    void SetOwner(AActor* InOwner);

    AActor* Owner = nullptr; // Non-owning reference.
    bool bTickEnabled = false;
};
