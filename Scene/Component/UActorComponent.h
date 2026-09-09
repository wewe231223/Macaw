#pragma once

#include "Core/Base/UObject.h"

class AActor;
class FArchive;
class UActorComponent : public UObject
{
public:
    UActorComponent() = default;
    ~UActorComponent() override = default;

    JG_DECLARE_DERIVED_TYPEINFO(UActorComponent, UObject)

    AActor* GetOwner() const;

    virtual void OnCreate();
    virtual void Tick(float DeltaTime);
    virtual void OnDestroy();

    bool IsActive() const;
    void SetActive(bool bInActive);

protected:
    void Serialize(FArchive& Archive) override;

private:
    friend class AActor;

    void SetOwner(AActor* InOwner);

private:
    AActor* Owner = nullptr;
    bool bActive = true;
};
