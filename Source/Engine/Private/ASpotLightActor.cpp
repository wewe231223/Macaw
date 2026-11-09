/**
 * @file ASpotLightActor.cpp
 * @brief 스포트라이트 컴포넌트를 기본으로 생성하는 액터입니다.
 * @ingroup Engine
 */

#include "Engine/ASpotLightActor.h"
#include "Engine/Component/USpotLightComponent.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/Cast.h"

namespace Engine {

    /**
     * @brief ASpotLightActor의 기본 상태를 초기화합니다.
     */
    Engine::ASpotLightActor::ASpotLightActor() = default;

    /**
     * @brief ASpotLightActor 객체의 수명을 끝내고 소유 자원을 정리합니다.
     */
    Engine::ASpotLightActor::~ASpotLightActor() = default;

    /**
     * @brief 액터 종류에 맞는 기본 광원 컴포넌트를 생성하고 루트에 연결합니다.
     */
    bool Engine::ASpotLightActor::InitializeDefaultComponents() {
        if (!GetComponents().empty()) {
            return false;
        }

        auto Component{CoreUObject::NewObject<Engine::USpotLightComponent>()};
        Component->SetName("SpotLightComponent");

        return AddComponent(std::move(Component));
    }

    /**
     * @brief 액터의 기본 광원 컴포넌트를 조회합니다.
     */
    Engine::USpotLightComponent* Engine::ASpotLightActor::GetLightComponent() const {
        return CoreUObject::Cast<Engine::USpotLightComponent>(GetRootComponent());
    }

}
