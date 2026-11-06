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

    /**
     * @brief 액터 종류에 맞는 기본 광원 컴포넌트를 생성하고 루트에 연결합니다.
     */
    bool Engine::ADirectionalLightActor::InitializeDefaultComponents() {
        if (!GetComponents().empty()) {
            return false;
        }

        auto Component{CoreUObject::NewObject<Engine::UDirectionalLightComponent>()};
        Component->SetName("DirectionalLightComponent");

        return AddComponent(std::move(Component));
    }

    /**
     * @brief 액터의 기본 광원 컴포넌트를 조회합니다.
     */
    Engine::UDirectionalLightComponent* Engine::ADirectionalLightActor::GetLightComponent() const {
        return CoreUObject::Cast<Engine::UDirectionalLightComponent>(GetRootComponent());
    }

}
