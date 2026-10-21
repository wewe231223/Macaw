#include "Engine/AActor.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/UWorld.h"
#include "Core/Math/FTransform.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include <utility>

AActor::AActor() = default;

AActor::~AActor() {
    SetWorld(nullptr);
}

ImplementReflection(AActor, EClassFlags::NoFactory) {
    static_cast<void>(Information);
}

UWorld* AActor::GetWorld() const {
    return mWorld;
}

void AActor::SetWorld(UWorld* World) {
    if (mWorld == World) {
        return;
    }

    for (const auto& Component : mComponents) {
        Component->UnregisterComponent();
    }

    mWorld = World;

    if (mWorld != nullptr) {
        for (const auto& Component : mComponents) {
            Component->RegisterComponent(*mWorld);
        }
    }
}

bool AActor::AddComponent(std::unique_ptr<UActorComponent> Component) {
    if (Component == nullptr || Component->GetOwner() != nullptr || Component->GetRegistry() != GetRegistry()) {
        return false;
    }

    Component->SetOwner(this);

    if (mRootComponent == nullptr) {
        mRootComponent = Cast<USceneComponent>(Component.get());
    }

    if (mWorld != nullptr) {
        Component->RegisterComponent(*mWorld);
    }

    mComponents.push_back(std::move(Component));

    return true;
}

const TArray<std::unique_ptr<UActorComponent>>& AActor::GetComponents() const {
    return mComponents;
}

USceneComponent* AActor::GetRootComponent() const {
    return mRootComponent;
}

bool AActor::SetActorTransform(const FTransform& Transform) {
    return mRootComponent != nullptr && mRootComponent->SetTransform(Transform);
}

bool AActor::InitializeDefaultComponents() {
    return false;
}
