#pragma once
#include "CoreUObject/UObject.h"
#include "World/Subsystem/UWorldSubsystem.generated.h"

class UWorld;

/// <summary>A World-owned RAII service that follows the World lifetime.</summary>
UCLASS(Abstract)
class UWorldSubsystem : public UObject {
public:
    GENERATED_TYPES(UWorldSubsystem)

public:
    UWorldSubsystem();
    ~UWorldSubsystem() override = default;

public:
    GENERATED_BODY(UWorldSubsystem)

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
