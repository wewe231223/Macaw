#pragma once

#include "Core/Base/UObject.h"

class AActor;

class UActorComponent : public UObject
{
public:
    UActorComponent() = default;
    ~UActorComponent() override = default;

    AActor* GetOwner() const;

    virtual void OnCreate();
    virtual void Tick(float DeltaTime);
    virtual void OnDestroy();

    bool IsTickEnabled() const;
    void SetTickEnabled(bool bInTickEnabled);

private:
    friend class AActor;

    void SetOwner(AActor* InOwner);

    AActor* Owner = nullptr; 
    bool bTickEnabled = false;
};
