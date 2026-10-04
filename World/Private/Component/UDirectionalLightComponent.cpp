#include "pch.h"
#include "World/Component/UDirectionalLightComponent.h"

ELightType UDirectionalLightComponent::GetLightType() const {
    return ELightType::Directional;
}

const FTypeInfo* UDirectionalLightComponent::StaticTypeInfo() noexcept {
    static const FTypeInfo Information{"UDirectionalLightComponent", ULightComponent::StaticTypeInfo(), +[]() -> std::unique_ptr<UObject> {
        return std::make_unique<UDirectionalLightComponent>();
    }};
    return &Information;
}

const FTypeInfo* UDirectionalLightComponent::GetTypeInfo() const noexcept {
    return StaticTypeInfo();
}
