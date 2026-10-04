#include "pch.h"
#include "World/Component/UPointLightComponent.h"

ELightType UPointLightComponent::GetLightType() const {
    return ELightType::Point;
}

const FTypeInfo* UPointLightComponent::StaticTypeInfo() noexcept {
    static const FTypeInfo Information{"UPointLightComponent", ULocalLightComponent::StaticTypeInfo(), +[]() -> std::unique_ptr<UObject> {
        return std::make_unique<UPointLightComponent>();
    }};
    return &Information;
}

const FTypeInfo* UPointLightComponent::GetTypeInfo() const noexcept {
    return StaticTypeInfo();
}
