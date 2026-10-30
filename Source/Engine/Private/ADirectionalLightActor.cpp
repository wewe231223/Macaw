/**
 * @file ADirectionalLightActor.cpp
 * @brief 방향광 컴포넌트를 기본으로 생성하는 액터입니다.
 * @ingroup Engine
 */

#include "Engine/ADirectionalLightActor.h"
#include "Engine/Component/UDirectionalLightComponent.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"

namespace Engine {

    Engine::ADirectionalLightActor::ADirectionalLightActor() = default;
    Engine::ADirectionalLightActor::~ADirectionalLightActor() = default;

    bool Engine::ADirectionalLightActor::InitializeDefaultComponents() {
        if (!GetComponents().empty()) {
            return false;
        }

        auto Component{CoreUObject::NewObject<Engine::UDirectionalLightComponent>()};
        Component->SetName("DirectionalLightComponent");

        return AddComponent(std::move(Component));
    }

    Engine::UDirectionalLightComponent* Engine::ADirectionalLightActor::GetLightComponent() const {
        return CoreUObject::Cast<Engine::UDirectionalLightComponent>(GetRootComponent());
    }

}
