/**
 * @file UPointLightComponent.cpp
 * @brief 반경과 감쇠 지수를 갖는 점광원을 관리합니다.
 * @ingroup Engine
 */

#include "Engine/Component/UPointLightComponent.h"

#include <cmath>

UPointLightComponent::UPointLightComponent() = default;
UPointLightComponent::~UPointLightComponent() = default;

FLightSceneProxy UPointLightComponent::CreateLightProxy() const {
    FLightSceneProxy Proxy{ULightComponent::CreateLightProxy()};
    Proxy.mType = ELightType::Point;
    Proxy.mAttenuationRadius = mAttenuationRadius;
    Proxy.mLightFalloffExponent = mLightFalloffExponent;

    return Proxy;
}

float UPointLightComponent::GetAttenuationRadius() const {
    return mAttenuationRadius;
}

bool UPointLightComponent::SetAttenuationRadius(float AttenuationRadius) {
    if (!std::isfinite(AttenuationRadius) || AttenuationRadius <= 0.0f) {
        return false;
    }

    mAttenuationRadius = AttenuationRadius;
    SynchronizeLight();

    return true;
}

float UPointLightComponent::GetLightFalloffExponent() const {
    return mLightFalloffExponent;
}

bool UPointLightComponent::SetLightFalloffExponent(float LightFalloffExponent) {
    if (!std::isfinite(LightFalloffExponent) || LightFalloffExponent < 0.0f) {
        return false;
    }

    mLightFalloffExponent = LightFalloffExponent;
    SynchronizeLight();

    return true;
}
