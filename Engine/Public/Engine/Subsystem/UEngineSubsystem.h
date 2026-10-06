#pragma once

#include "CoreUObject/UObject.h"

class UEngine;

class UEngineSubsystem : public UObject {
public:
    UEngineSubsystem() = default;
    ~UEngineSubsystem() override = default;

public:
    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(UEngineSubsystem, UObject)

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
