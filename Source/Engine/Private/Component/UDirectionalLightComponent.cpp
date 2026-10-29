#include "Engine/Component/UDirectionalLightComponent.h"

#include <cmath>

UDirectionalLightComponent::UDirectionalLightComponent() = default;
UDirectionalLightComponent::~UDirectionalLightComponent() = default;

FLightSceneProxy UDirectionalLightComponent::CreateLightProxy() const {
    FLightSceneProxy Proxy{ULightComponent::CreateLightProxy()};
    Proxy.mType = ELightType::Directional;
    Proxy.mCascadeCount = mCascadeCount;
    Proxy.mShadowDistance = mShadowDistance;
    Proxy.mCascadeTransitionFraction = mCascadeTransitionFraction;
    return Proxy;
}

Uint32 UDirectionalLightComponent::GetCascadeCount() const {
    return mCascadeCount;
}

bool UDirectionalLightComponent::SetCascadeCount(Uint32 Count) {
    if (Count < 1 || Count > 4) {
        return false;
    }
    mCascadeCount = Count;
    SynchronizeLight();
    return true;
}

float UDirectionalLightComponent::GetShadowDistance() const {
    return mShadowDistance;
}

bool UDirectionalLightComponent::SetShadowDistance(float Distance) {
    if (!std::isfinite(Distance) || Distance < 1.0f || Distance > 100000.0f) {
        return false;
    }
    mShadowDistance = Distance;
    SynchronizeLight();
    return true;
}

float UDirectionalLightComponent::GetCascadeTransitionFraction() const {
    return mCascadeTransitionFraction;
}

bool UDirectionalLightComponent::SetCascadeTransitionFraction(float Fraction) {
    if (!std::isfinite(Fraction) || Fraction < 0.0f || Fraction > 0.5f) {
        return false;
    }
    mCascadeTransitionFraction = Fraction;
    SynchronizeLight();
    return true;
}
