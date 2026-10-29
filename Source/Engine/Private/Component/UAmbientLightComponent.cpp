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
