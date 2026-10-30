/**
 * @file APointLightActor.cpp
 * @brief 점광원 컴포넌트를 기본으로 생성하는 액터입니다.
 * @ingroup Engine
 */

#include "Engine/APointLightActor.h"
#include "Engine/Component/UPointLightComponent.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"

namespace Engine {

    Engine::APointLightActor::APointLightActor() = default;
    Engine::APointLightActor::~APointLightActor() = default;

    bool Engine::APointLightActor::InitializeDefaultComponents() {
        if (!GetComponents().empty()) {
            return false;
        }

        auto Component{CoreUObject::NewObject<Engine::UPointLightComponent>()};
        Component->SetName("PointLightComponent");

        return AddComponent(std::move(Component));
    }

    Engine::UPointLightComponent* Engine::APointLightActor::GetLightComponent() const {
        return CoreUObject::Cast<Engine::UPointLightComponent>(GetRootComponent());
    }

}
