#pragma once

#include "CoreUObject/UObject.h"
#include "Engine/Subsystem/UEngineSubsystem.generated.h"

class UEngine;

UCLASS(Abstract)
class UEngineSubsystem : public UObject {
public:
    GENERATED_TYPES(UEngineSubsystem)

public:
    UEngineSubsystem();
    ~UEngineSubsystem() override = default;

public:
    GENERATED_BODY(UEngineSubsystem)

    void Initialize(UEngine* Engine);
    void Deinitialize();
    UEngine* GetEngine() const;
    bool IsInitialized() const;

private:
    bool CanChangeOuter(const UObject* NewOuter) const override;
    virtual void OnInitialize();
    virtual void OnDeinitialize();

private:
    UEngine* mEngine{};
};
