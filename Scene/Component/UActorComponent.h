#pragma once 

#include "Core/Base/UObject.h"

class AActor;

class UActorComponent : public UObject
{
public:
    UActorComponent() = default;
    ~UActorComponent() override = default;

    AActor* GetOwner() const;

    bool IsActive() const;
    void SetActive(bool bInActive);

private:
    friend class AActor;

    void SetOwner(AActor* InOwner);

private:
    AActor* Owner = nullptr;
    bool bActive = true;
};