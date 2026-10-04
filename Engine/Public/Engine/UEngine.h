#pragma once

#include <memory>
#include <vector>
#include "CoreUObject/TSubsystemCollection.h"
#include "Engine/FWorldContext.h"
#include "Engine/Subsystem/UEngineSubsystem.h"

class FAssetRegistry;

class UEngine : public UObject {
public:
    UEngine();
    ~UEngine() override;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UEngine, UObject)

    virtual void Initialize();
    virtual void Shutdown();
    bool IsInitialized() const;
    void Tick(float DeltaTime);

    FWorldContext& CreateWorldContext(EWorldType WorldType);
    bool DestroyWorldContext(FWorldContext& Context);
    const std::vector<std::unique_ptr<FWorldContext>>& GetWorldContexts() const;
    FAssetRegistry& GetAssetRegistry();
    TSubsystemCollection<UEngineSubsystem, UEngine>& GetSubsystems();

private:
    void RegisterObjectTypes();

private:
    std::unique_ptr<FAssetRegistry> mAssetRegistry{};
    TSubsystemCollection<UEngineSubsystem, UEngine> mSubsystems{};
    std::vector<std::unique_ptr<FWorldContext>> mWorldContexts{};
    bool mInitialized{};
    bool mTicking{};
};
