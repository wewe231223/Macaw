#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include "CoreUObject/UObject.h"

class UEngine;

class UEngineSubsystem : public UObject {
public:
    ReflectTypes(UEngineSubsystem, UObject);

public:
    UEngineSubsystem();
    ~UEngineSubsystem() override = default;

public:
    ReflectClass(UEngineSubsystem, EClassFlags::Abstract);

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
