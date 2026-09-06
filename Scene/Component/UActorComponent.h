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

    bool IsActive() const;
    void SetActive(bool bInActive);

private:
    friend class AActor;

    void SetOwner(AActor* InOwner);

private:
    AActor* Owner = nullptr;
    bool bActive = true;
};
