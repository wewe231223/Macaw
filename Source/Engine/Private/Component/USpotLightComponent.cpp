/**
 * @file USpotLightComponent.cpp
 * @brief 내부·외부 원뿔각으로 제한되는 스포트라이트를 관리합니다.
 * @ingroup Engine
 */

#include "Engine/Component/USpotLightComponent.h"

#include <algorithm>
#include <cmath>

USpotLightComponent::USpotLightComponent() = default;
USpotLightComponent::~USpotLightComponent() = default;

FLightSceneProxy USpotLightComponent::CreateLightProxy() const {
    FLightSceneProxy Proxy{UPointLightComponent::CreateLightProxy()};
    Proxy.mType = ELightType::Spot;
    Proxy.mInnerConeAngle = mInnerConeAngle;
    Proxy.mOuterConeAngle = mOuterConeAngle;

    return Proxy;
}

float USpotLightComponent::GetInnerConeAngle() const {
    return mInnerConeAngle;
}

bool USpotLightComponent::SetInnerConeAngle(float InnerConeAngle) {
    if (!std::isfinite(InnerConeAngle) || InnerConeAngle < 0.0f || InnerConeAngle >= 89.0f) {
        return false;
    }

    mInnerConeAngle = InnerConeAngle;
    mOuterConeAngle = std::max(mOuterConeAngle, mInnerConeAngle);
    SynchronizeLight();

    return true;
}

float USpotLightComponent::GetOuterConeAngle() const {
    return mOuterConeAngle;
}

bool USpotLightComponent::SetOuterConeAngle(float OuterConeAngle) {
    if (!std::isfinite(OuterConeAngle) || OuterConeAngle < 0.0f || OuterConeAngle >= 89.0f) {
        return false;
    }

    mOuterConeAngle = OuterConeAngle;
    mInnerConeAngle = std::min(mInnerConeAngle, mOuterConeAngle);
    SynchronizeLight();

    return true;
}
