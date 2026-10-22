#include "Engine/AStaticMeshActor.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include <utility>

AStaticMeshActor::AStaticMeshActor() = default;

AStaticMeshActor::~AStaticMeshActor() = default;

ImplementReflection(AStaticMeshActor)

bool AStaticMeshActor::InitializeDefaultComponents() {
    if (GetRegistry() == nullptr || !GetComponents().empty()) {
        return false;
    }

    auto Component{NewObject<UStaticMeshComponent>(*GetRegistry())};
    Component->SetName("StaticMeshComponent");

    return AddComponent(std::move(Component));
}

UStaticMeshComponent* AStaticMeshActor::GetStaticMeshComponent() const {
    return Cast<UStaticMeshComponent>(GetRootComponent());
}
