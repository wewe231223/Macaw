#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "CoreUObject/UObject.h"

class UWorld;

/// <summary>A World-owned RAII service that follows the World lifetime.</summary>
class UWorldSubsystem : public UObject {
public:
    ReflectTypes(UWorldSubsystem, UObject);

public:
    UWorldSubsystem();
    ~UWorldSubsystem() override = default;

public:
    ReflectClass(UWorldSubsystem, EClassFlags::Abstract);

    void Initialize(UWorld* World);
    void Deinitialize();

    UWorld* GetWorld() const;
    bool IsInitialized() const;

private:
    bool CanChangeOuter(const UObject* NewOuter) const override;

    virtual void OnInitialize();
    virtual void OnDeinitialize();

private:
    UWorld* mWorld{nullptr};
    bool mBInitialized{false};
};
