#include "pch.h"

#include "UPointLightComponent.h"

ELightType UPointLightComponent::GetLightType() const {
    return ELightType::Point;
}
