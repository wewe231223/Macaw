#include "Engine/Component/ULightComponent.h"

#include <cmath>

ULightComponent::ULightComponent() = default;
ULightComponent::~ULightComponent() = default;

bool ULightComponent::GetCastShadows() const {
    return mCastShadows;
}

bool ULightComponent::SetCastShadows(bool CastShadows) {
    mCastShadows = CastShadows;
    SynchronizeLight();
    return true;
}

Uint32 ULightComponent::GetShadowResolution() const {
    return mShadowResolution;
}

bool ULightComponent::SetShadowResolution(Uint32 Resolution) {
    if (Resolution < 64 || Resolution > 8192 || (Resolution & (Resolution - 1)) != 0) {
        return false;
    }
    mShadowResolution = Resolution;
    SynchronizeLight();
    return true;
}

float ULightComponent::GetShadowResolutionScale() const {
    return mShadowResolutionScale;
}

bool ULightComponent::SetShadowResolutionScale(float Scale) {
    if (!std::isfinite(Scale) || Scale < 0.125f || Scale > 8.0f) {
        return false;
    }
    mShadowResolutionScale = Scale;
    SynchronizeLight();
    return true;
}

float ULightComponent::GetShadowBias() const {
    return mShadowBias;
}

bool ULightComponent::SetShadowBias(float Bias) {
    if (!std::isfinite(Bias) || Bias < 0.0f || Bias > 0.1f) {
        return false;
    }
    mShadowBias = Bias;
    SynchronizeLight();
    return true;
}

float ULightComponent::GetShadowSlopeBias() const {
    return mShadowSlopeBias;
}

bool ULightComponent::SetShadowSlopeBias(float Bias) {
    if (!std::isfinite(Bias) || Bias < 0.0f || Bias > 16.0f) {
        return false;
    }
    mShadowSlopeBias = Bias;
    SynchronizeLight();
    return true;
}

float ULightComponent::GetShadowSharpen() const {
    return mShadowSharpen;
}

bool ULightComponent::SetShadowSharpen(float Sharpen) {
    if (!std::isfinite(Sharpen) || Sharpen < 0.0f || Sharpen > 10.0f) {
        return false;
    }
    mShadowSharpen = Sharpen;
    SynchronizeLight();
    return true;
}

FLightSceneProxy ULightComponent::CreateLightProxy() const {
    FLightSceneProxy Proxy{ULightComponentBase::CreateLightProxy()};
    Proxy.mCastShadows = mCastShadows;
    Proxy.mShadowResolution = mShadowResolution;
    Proxy.mShadowResolutionScale = mShadowResolutionScale;
    Proxy.mShadowBias = mShadowBias;
    Proxy.mShadowSlopeBias = mShadowSlopeBias;
    Proxy.mShadowSharpen = mShadowSharpen;
    return Proxy;
}
