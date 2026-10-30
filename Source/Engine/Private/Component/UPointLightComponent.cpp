/**
 * @file UPointLightComponent.cpp
 * @brief 반경과 감쇠 지수를 갖는 점광원을 관리합니다.
 * @ingroup Engine
 */

#include "Engine/Component/UPointLightComponent.h"

#include <cmath>

namespace Engine {

    Engine::UPointLightComponent::UPointLightComponent() = default;
    Engine::UPointLightComponent::~UPointLightComponent() = default;

    Engine::FLightSceneProxy Engine::UPointLightComponent::CreateLightProxy() const {
        Engine::FLightSceneProxy Proxy{Engine::ULightComponent::CreateLightProxy()};
        Proxy.mType = Engine::ELightType::Point;
        Proxy.mAttenuationRadius = mAttenuationRadius;
        Proxy.mLightFalloffExponent = mLightFalloffExponent;

        return Proxy;
    }

    float Engine::UPointLightComponent::GetAttenuationRadius() const {
        return mAttenuationRadius;
    }

    bool Engine::UPointLightComponent::SetAttenuationRadius(float AttenuationRadius) {
        if (!std::isfinite(AttenuationRadius) || AttenuationRadius <= 0.0f) {
            return false;
        }

        mAttenuationRadius = AttenuationRadius;
        SynchronizeLight();

        return true;
    }

    float Engine::UPointLightComponent::GetLightFalloffExponent() const {
        return mLightFalloffExponent;
    }

    bool Engine::UPointLightComponent::SetLightFalloffExponent(float LightFalloffExponent) {
        if (!std::isfinite(LightFalloffExponent) || LightFalloffExponent < 0.0f) {
            return false;
        }

        mLightFalloffExponent = LightFalloffExponent;
        SynchronizeLight();

        return true;
    }

}
