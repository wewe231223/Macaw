#include "Engine/Component/UDirectionalLightComponent.h"

#include <cmath>

UDirectionalLightComponent::UDirectionalLightComponent() = default;
UDirectionalLightComponent::~UDirectionalLightComponent() = default;

FLightSceneProxy UDirectionalLightComponent::CreateLightProxy() const {
    FLightSceneProxy Proxy{ULightComponent::CreateLightProxy()};
    Proxy.mType = ELightType::Directional;
    return Proxy;
}
