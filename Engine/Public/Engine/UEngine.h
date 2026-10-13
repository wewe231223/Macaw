#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include <memory>
#include <vector>
#include "CoreUObject/TSubsystemCollection.h"
#include "Engine/FWorldContext.h"
#include "Engine/Subsystem/UEngineSubsystem.h"
#include "World/Tick/ETickType.h"

class FAssetRegistry;

class UEngine : public UObject {
public:
    ReflectTypes(UEngine, UObject);

public:
    UEngine();
    ~UEngine() override;

public:
    ReflectClass(UEngine);

    virtual void Initialize();
    virtual void Shutdown();
    bool IsInitialized() const;
    virtual void Tick(float DeltaTime);
    bool IsTicking() const;
    bool CanChangeWorldContexts() const;
    virtual ETickType GetWorldTickType(const FWorldContext& Context) const;

    FWorldContext& CreateWorldContext(EWorldType WorldType);
    FWorldContext* AdoptWorld(std::unique_ptr<UWorld>& World);
    virtual bool DestroyWorldContext(FWorldContext& Context);
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
