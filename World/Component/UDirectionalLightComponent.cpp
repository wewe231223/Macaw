#include "pch.h"

#include "UDirectionalLightComponent.h"

ELightType UDirectionalLightComponent::GetLightType() const {
    return ELightType::Directional;
}
