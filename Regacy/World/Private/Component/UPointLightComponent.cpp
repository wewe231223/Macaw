#include "pch.h"
#include "World/Component/UPointLightComponent.h"

ELightType UPointLightComponent::GetLightType() const {
    return ELightType::Point;
}

UPointLightComponent::UPointLightComponent() {
    InitializeClass(StaticClass());
}
