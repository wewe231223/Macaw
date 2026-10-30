/**
 * @file ADirectionalLightActor.cpp
 * @brief 방향광 컴포넌트를 기본으로 생성하는 액터입니다.
 * @ingroup Engine
 */

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
