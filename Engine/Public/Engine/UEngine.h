#pragma once

#include <memory>
#include <vector>
#include "CoreUObject/TSubsystemCollection.h"
#include "Engine/FWorldContext.h"
#include "Engine/Subsystem/UEngineSubsystem.h"
#include "Engine/UEngine.generated.h"

class FAssetRegistry;

UCLASS()
class UEngine : public UObject {
public:
    GENERATED_TYPES(UEngine)

public:
    UEngine();
    ~UEngine() override;

public:
    GENERATED_BODY(UEngine)

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
