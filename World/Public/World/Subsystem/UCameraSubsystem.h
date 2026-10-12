#pragma once
#include "World/Subsystem/UWorldSubsystem.h"
#include "World/Component/UCameraComponent.h"
#include "World/Subsystem/UCameraSubsystem.generated.h"

/// <summary>Maintains the World's active main camera.</summary>
UCLASS()
class UCameraSubsystem : public UWorldSubsystem {
public:
    GENERATED_TYPES(UCameraSubsystem)

public:
    UCameraSubsystem();
    ~UCameraSubsystem() override = default;

public:
    GENERATED_BODY(UCameraSubsystem)

    void SetMainCamera(UCameraComponent* Camera);
    void ClearMainCamera(UCameraComponent* Camera);
    UCameraComponent* GetMainCamera() const;

protected:
    void OnDeinitialize() override;

private:
    UCameraComponent* mMainCamera{nullptr};
};
