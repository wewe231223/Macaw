/**
 * @file UDirectionalLightComponent.cpp
 * @brief 방향광과 캐스케이드 그림자 설정을 관리합니다.
 * @ingroup Engine
 */

#include "Engine/Component/UDirectionalLightComponent.h"

#include <cmath>

namespace Engine {

    Engine::UDirectionalLightComponent::UDirectionalLightComponent() = default;
    Engine::UDirectionalLightComponent::~UDirectionalLightComponent() = default;

    /**
     * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
     */
    Engine::FLightSceneProxy Engine::UDirectionalLightComponent::CreateLightProxy() const {
        Engine::FLightSceneProxy Proxy{Engine::ULightComponent::CreateLightProxy()};
        Proxy.mType = Engine::ELightType::Directional;
        Proxy.mCascadeCount = mCascadeCount;
        Proxy.mShadowDistance = mShadowDistance;
        Proxy.mCascadeTransitionFraction = mCascadeTransitionFraction;
        Proxy.mShadowSourceAngle = mShadowSourceAngle;

        return Proxy;
    }

    /**
     * @brief 방향광 그림자의 캐스케이드 개수를 반환합니다.
     */
    Core::Uint32 Engine::UDirectionalLightComponent::GetCascadeCount() const {
        return mCascadeCount;
    }

    /**
     * @brief 방향광 그림자의 캐스케이드 개수를 검증하고 변경합니다.
     */
    bool Engine::UDirectionalLightComponent::SetCascadeCount(Core::Uint32 Count) {
        if (Count < 1 || Count > 4) {
            return false;
        }

        mCascadeCount = Count;

        SynchronizeLight();

        return true;
    }

    /**
     * @brief 방향광 그림자의 최대 표시 거리를 반환합니다.
     */
    float Engine::UDirectionalLightComponent::GetShadowDistance() const {
        return mShadowDistance;
    }

    /**
     * @brief 방향광 그림자의 최대 표시 거리를 검증하고 변경합니다.
     */
    bool Engine::UDirectionalLightComponent::SetShadowDistance(float Distance) {
        if (!std::isfinite(Distance) || Distance < 1.0f || Distance > 100000.0f) {
            return false;
        }

        mShadowDistance = Distance;

        SynchronizeLight();

        return true;
    }

    /**
     * @brief 각 캐스케이드 구간에서 전환에 사용하는 비율을 반환합니다.
     */
    float Engine::UDirectionalLightComponent::GetCascadeTransitionFraction() const {
        return mCascadeTransitionFraction;
    }

    /**
     * @brief 각 캐스케이드 구간에서 전환에 사용하는 비율을 검증하고 변경합니다.
     */
    bool Engine::UDirectionalLightComponent::SetCascadeTransitionFraction(float Fraction) {
        if (!std::isfinite(Fraction) || Fraction < 0.0f || Fraction > 0.5f) {
            return false;
        }

        mCascadeTransitionFraction = Fraction;

        SynchronizeLight();

        return true;
    }

    /**
     * @brief PCSS 반그림자 계산에 쓰는 광원 각반지름을 반환합니다.
     */
    float Engine::UDirectionalLightComponent::GetShadowSourceAngle() const {
        return mShadowSourceAngle;
    }

    /**
     * @brief PCSS 반그림자 계산에 쓰는 광원 각반지름을 검증하고 변경합니다.
     */
    bool Engine::UDirectionalLightComponent::SetShadowSourceAngle(float Angle) {
        if (!std::isfinite(Angle) || Angle < 0.0f || Angle > 15.0f) {
            return false;
        }

        mShadowSourceAngle = Angle;

        SynchronizeLight();

        return true;
    }

}
