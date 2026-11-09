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

    /**
     * @brief AAmbientLightActor의 기본 상태를 초기화합니다.
     */
    Engine::AAmbientLightActor::AAmbientLightActor() = default;

    /**
     * @brief AAmbientLightActor 객체의 수명을 끝내고 소유 자원을 정리합니다.
     */
    Engine::AAmbientLightActor::~AAmbientLightActor() = default;

    /**
     * @brief 액터 종류에 맞는 기본 광원 컴포넌트를 생성하고 루트에 연결합니다.
     */
    bool Engine::AAmbientLightActor::InitializeDefaultComponents() {
        if (!GetComponents().empty()) {
            return false;
        }

        auto Component{CoreUObject::NewObject<Engine::UAmbientLightComponent>()};
        Component->SetName("AmbientLightComponent");

        return AddComponent(std::move(Component));
    }

    /**
     * @brief 액터의 기본 광원 컴포넌트를 조회합니다.
     */
    Engine::UAmbientLightComponent* Engine::AAmbientLightActor::GetLightComponent() const {
        return CoreUObject::Cast<Engine::UAmbientLightComponent>(GetRootComponent());
    }

}
