#include "Engine/ADirectionalLightActor.h"
#include "Engine/Component/UDirectionalLightComponent.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"

ADirectionalLightActor::ADirectionalLightActor() = default;
ADirectionalLightActor::~ADirectionalLightActor() = default;

bool ADirectionalLightActor::InitializeDefaultComponents() {
    if (!GetComponents().empty()) {
        return false;
    }
    auto Component{NewObject<UDirectionalLightComponent>()};
    Component->SetName("DirectionalLightComponent");
    return AddComponent(std::move(Component));
}

UDirectionalLightComponent* ADirectionalLightActor::GetLightComponent() const {
    return Cast<UDirectionalLightComponent>(GetRootComponent());
}
