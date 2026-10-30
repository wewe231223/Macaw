/**
 * @file UAmbientLightComponent.cpp
 * @brief 위치와 방향에 무관한 환경광을 렌더 장면에 전달합니다.
 * @ingroup Engine
 */

#include "Engine/Component/UAmbientLightComponent.h"

#include <cmath>

UAmbientLightComponent::UAmbientLightComponent() {
    SetIntensity(0.15f);
}
UAmbientLightComponent::~UAmbientLightComponent() = default;

FLightSceneProxy UAmbientLightComponent::CreateLightProxy() const {
    FLightSceneProxy Proxy{ULightComponent::CreateLightProxy()};
    Proxy.mType = ELightType::Ambient;
    return Proxy;
}
