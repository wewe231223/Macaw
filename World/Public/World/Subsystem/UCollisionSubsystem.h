#pragma once
#include "World/Subsystem/UWorldSubsystem.h"
#include "CoreUObject/TObjectRef.h"
#include "World/Component/UCollisionComponent.h"
#include "World/Subsystem/UCollisionSubsystem.generated.h"

/// <summary>Provides world-space raycasts over registered CollisionComponents.</summary>
UCLASS()
class UCollisionSubsystem : public UWorldSubsystem {
public:
    GENERATED_TYPES(UCollisionSubsystem)

public:
    UCollisionSubsystem();
    ~UCollisionSubsystem() override = default;

public:
    GENERATED_BODY(UCollisionSubsystem)

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
