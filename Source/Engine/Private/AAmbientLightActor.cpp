/**
 * @file AAmbientLightActor.cpp
 * @brief 환경광 컴포넌트를 기본으로 생성하는 액터입니다.
 * @ingroup Engine
 */

#include "Engine/AAmbientLightActor.h"
#include "Engine/Component/UAmbientLightComponent.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"

AAmbientLightActor::AAmbientLightActor() = default;
AAmbientLightActor::~AAmbientLightActor() = default;

bool AAmbientLightActor::InitializeDefaultComponents() {
    if (!GetComponents().empty()) {
        return false;
    }
    auto Component{NewObject<UAmbientLightComponent>()};
    Component->SetName("AmbientLightComponent");
    return AddComponent(std::move(Component));
}

UAmbientLightComponent* AAmbientLightActor::GetLightComponent() const {
    return Cast<UAmbientLightComponent>(GetRootComponent());
}
