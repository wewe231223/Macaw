#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Subsystem/UWorldSubsystem.h"
#include "CoreUObject/TObjectRef.h"
#include "World/Component/UCollisionComponent.h"

/// <summary>Provides world-space raycasts over registered CollisionComponents.</summary>
class UCollisionSubsystem : public UWorldSubsystem {
public:
    ReflectTypes(UCollisionSubsystem, UWorldSubsystem);

public:
    UCollisionSubsystem();
    ~UCollisionSubsystem() override = default;

public:
    ReflectClass(UCollisionSubsystem);

    void RegisterComponent(UCollisionComponent* Component);
    void UnregisterComponent(UCollisionComponent* Component);
    bool Raycast(const FRay& Ray, UCollisionComponent*& OutComponent, float& OutDistance) const;

    bool ContainsComponent(const UCollisionComponent* Component) const;
    const TArray<TObjectRef<UCollisionComponent>>& GetRegisteredComponents() const;

protected:
    void OnDeinitialize() override;

private:
    TArray<TObjectRef<UCollisionComponent>> mComponents{};
};
