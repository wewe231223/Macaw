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

    /**
     * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
     */
    Engine::FLightSceneProxy Engine::UPointLightComponent::CreateLightProxy() const {
        Engine::FLightSceneProxy Proxy{Engine::ULightComponent::CreateLightProxy()};
        Proxy.mType = Engine::ELightType::Point;
        Proxy.mAttenuationRadius = mAttenuationRadius;
        Proxy.mLightFalloffExponent = mLightFalloffExponent;

        return Proxy;
    }

    /**
     * @brief 점광원·스포트라이트의 영향 반경을 반환합니다.
     */
    float Engine::UPointLightComponent::GetAttenuationRadius() const {
        return mAttenuationRadius;
    }

    /**
     * @brief 점광원·스포트라이트의 영향 반경을 검증하고 변경합니다.
     */
    bool Engine::UPointLightComponent::SetAttenuationRadius(float AttenuationRadius) {
        if (!std::isfinite(AttenuationRadius) || AttenuationRadius <= 0.0f) {
            return false;
        }

        mAttenuationRadius = AttenuationRadius;

        SynchronizeLight();

        return true;
    }

    /**
     * @brief 거리에 따른 광원 감쇠 지수를 반환합니다.
     */
    float Engine::UPointLightComponent::GetLightFalloffExponent() const {
        return mLightFalloffExponent;
    }

    /**
     * @brief 거리에 따른 광원 감쇠 지수를 검증하고 변경합니다.
     */
    bool Engine::UPointLightComponent::SetLightFalloffExponent(float LightFalloffExponent) {
        if (!std::isfinite(LightFalloffExponent) || LightFalloffExponent < 0.0f) {
            return false;
        }

        mLightFalloffExponent = LightFalloffExponent;

        SynchronizeLight();

        return true;
    }

}
