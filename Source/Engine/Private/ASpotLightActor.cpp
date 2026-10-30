/**
 * @file ASpotLightActor.cpp
 * @brief 스포트라이트 컴포넌트를 기본으로 생성하는 액터입니다.
 * @ingroup Engine
 */

#include "Engine/ASpotLightActor.h"
#include "Engine/Component/USpotLightComponent.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"

ASpotLightActor::ASpotLightActor() = default;
ASpotLightActor::~ASpotLightActor() = default;

bool ASpotLightActor::InitializeDefaultComponents() {
    if (!GetComponents().empty()) {
        return false;
    }
    auto Component{NewObject<USpotLightComponent>()};
    Component->SetName("SpotLightComponent");
    return AddComponent(std::move(Component));
}

USpotLightComponent* ASpotLightActor::GetLightComponent() const {
    return Cast<USpotLightComponent>(GetRootComponent());
}
