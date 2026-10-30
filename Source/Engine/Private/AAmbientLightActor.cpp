/**
 * @file AAmbientLightActor.cpp
 * @brief 환경광 컴포넌트를 기본으로 생성하는 액터입니다.
 * @ingroup Engine
 */

#include "Engine/AAmbientLightActor.h"
#include "Engine/Component/UAmbientLightComponent.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"

namespace Engine {

    Engine::AAmbientLightActor::AAmbientLightActor() = default;
    Engine::AAmbientLightActor::~AAmbientLightActor() = default;

    bool Engine::AAmbientLightActor::InitializeDefaultComponents() {
        if (!GetComponents().empty()) {
            return false;
        }

        auto Component{CoreUObject::NewObject<Engine::UAmbientLightComponent>()};
        Component->SetName("AmbientLightComponent");

        return AddComponent(std::move(Component));
    }

    Engine::UAmbientLightComponent* Engine::AAmbientLightActor::GetLightComponent() const {
        return CoreUObject::Cast<Engine::UAmbientLightComponent>(GetRootComponent());
    }

}
