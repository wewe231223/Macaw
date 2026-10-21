#include "Engine/Component/UActorComponent.h"
#include "Engine/AActor.h"
#include "Engine/UWorld.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"

UActorComponent::UActorComponent() = default;

UActorComponent::~UActorComponent() = default;

ImplementReflection(UActorComponent, EClassFlags::NoFactory) {
    static_cast<void>(Information);
}

AActor* UActorComponent::GetOwner() const {
    return mOwner;
}

UWorld* UActorComponent::GetWorld() const {
    return mWorld;
}

void UActorComponent::SetOwner(AActor* Owner) {
    mOwner = Owner;
}

void UActorComponent::RegisterComponent(UWorld& World) {
    mWorld = &World;
}

void UActorComponent::UnregisterComponent() {
    mWorld = nullptr;
}

bool UActorComponent::IsRegistered() const {
    return mWorld != nullptr;
}
