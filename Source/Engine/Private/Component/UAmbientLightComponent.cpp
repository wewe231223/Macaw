/**
 * @file UAmbientLightComponent.cpp
 * @brief 위치와 방향에 무관한 환경광을 렌더 장면에 전달합니다.
 * @ingroup Engine
 */

#include "Engine/Component/UAmbientLightComponent.h"

#include <cmath>

namespace Engine {

    /**
     * @brief UAmbientLightComponent의 기본 상태를 초기화합니다.
     */
    Engine::UAmbientLightComponent::UAmbientLightComponent() {
        SetIntensity(0.15f);
    }

    /**
     * @brief UAmbientLightComponent 객체의 수명을 끝내고 소유 자원을 정리합니다.
     */
    Engine::UAmbientLightComponent::~UAmbientLightComponent() = default;

    /**
     * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
     */
    Engine::FLightSceneProxy Engine::UAmbientLightComponent::CreateLightProxy() const {
        Engine::FLightSceneProxy Proxy{Engine::ULightComponent::CreateLightProxy()};
        Proxy.mType = Engine::ELightType::Ambient;

        return Proxy;
    }

}
