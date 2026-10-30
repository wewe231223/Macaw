/**
 * @file UAmbientLightComponent.cpp
 * @brief 위치와 방향에 무관한 환경광을 렌더 장면에 전달합니다.
 * @ingroup Engine
 */

#include "Engine/Component/UAmbientLightComponent.h"

#include <cmath>

namespace Engine {

    Engine::UAmbientLightComponent::UAmbientLightComponent() {
        SetIntensity(0.15f);
    }

    Engine::UAmbientLightComponent::~UAmbientLightComponent() = default;

    Engine::FLightSceneProxy Engine::UAmbientLightComponent::CreateLightProxy() const {
        Engine::FLightSceneProxy Proxy{Engine::ULightComponent::CreateLightProxy()};
        Proxy.mType = Engine::ELightType::Ambient;
        return Proxy;
    }

}
