#include "pch.h"
#include "World/Component/UDirectionalLightComponent.h"

ELightType UDirectionalLightComponent::GetLightType() const {
    return ELightType::Directional;
}
