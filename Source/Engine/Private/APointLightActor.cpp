/**
 * @file APointLightActor.cpp
 * @brief 점광원 컴포넌트를 기본으로 생성하는 액터입니다.
 * @ingroup Engine
 */

#include "Engine/APointLightActor.h"
#include "Engine/Component/UPointLightComponent.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"

APointLightActor::APointLightActor() = default;
APointLightActor::~APointLightActor() = default;

bool APointLightActor::InitializeDefaultComponents() {
    if (!GetComponents().empty()) {
        return false;
    }
    auto Component{NewObject<UPointLightComponent>()};
    Component->SetName("PointLightComponent");
    return AddComponent(std::move(Component));
}

UPointLightComponent* APointLightActor::GetLightComponent() const {
    return Cast<UPointLightComponent>(GetRootComponent());
}
