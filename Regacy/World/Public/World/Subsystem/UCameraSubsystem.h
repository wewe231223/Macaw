#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Subsystem/UWorldSubsystem.h"
#include "World/Component/UCameraComponent.h"

/// <summary>Maintains the World's active main camera.</summary>
class UCameraSubsystem : public UWorldSubsystem {
public:
    ReflectTypes(UCameraSubsystem, UWorldSubsystem);

public:
    UCameraSubsystem();
    ~UCameraSubsystem() override = default;

public:
    ReflectClass(UCameraSubsystem);

    void SetMainCamera(UCameraComponent* Camera);
    void ClearMainCamera(UCameraComponent* Camera);
    UCameraComponent* GetMainCamera() const;

protected:
    void OnDeinitialize() override;

private:
    UCameraComponent* mMainCamera{nullptr};
};
