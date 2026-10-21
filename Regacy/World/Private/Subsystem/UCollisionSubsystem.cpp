#include "pch.h"
#include "World/Subsystem/UCollisionSubsystem.h"
#include "World/Component/UCollisionComponent.h"
#include "World/AActor.h"

void UCollisionSubsystem::RegisterComponent(UCollisionComponent* Component) {
    if (Component == nullptr || ContainsComponent(Component)) {
        return;
    }

    mComponents.emplace_back(Component);
}

void UCollisionSubsystem::UnregisterComponent(UCollisionComponent* Component) {
    std::erase_if(mComponents, [Component](const TObjectRef<UCollisionComponent>& ComponentRef) {
        return ComponentRef.Get() == Component;
    });
}

bool UCollisionSubsystem::Raycast(const FRay& Ray, const FWorldRaycastQuery& Query, FWorldRaycastHit& OutHit) const {
    OutHit = {};
    FRay WorldRay{};

    if (!Query.PrepareRay(Ray, WorldRay)) {
        return false;
    }

    float Distance{Query.mMaxDistance};

    for (const TObjectRef<UCollisionComponent>& ComponentRef : mComponents) {
        UCollisionComponent* Component{ComponentRef.Get()};

        if (Component == nullptr || Query.Excludes(*Component)) {
            continue;
        }

        FWorldRaycastHit Hit{};

        if (Component->Raycast(WorldRay, Hit, Distance) && (!OutHit.mComponent || Hit.mDistance < Distance)) {
            Distance = Hit.mDistance;
            OutHit = Hit;
        }
    }

    return OutHit.mComponent.IsValid();
}

bool UCollisionSubsystem::ContainsComponent(const UCollisionComponent* Component) const {
    return std::ranges::any_of(mComponents, [Component](const TObjectRef<UCollisionComponent>& ComponentRef) {
        return ComponentRef.Get() == Component;
    });
}

const TArray<TObjectRef<UCollisionComponent>>& UCollisionSubsystem::GetRegisteredComponents() const {
    return mComponents;
}

void UCollisionSubsystem::OnDeinitialize() {
    mComponents.clear();
}

UCollisionSubsystem::UCollisionSubsystem() {
    InitializeClass(StaticClass());
}
