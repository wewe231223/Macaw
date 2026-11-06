/**
 * @file USpotLightComponent.cpp
 * @brief 내부·외부 원뿔각으로 제한되는 스포트라이트를 관리합니다.
 * @ingroup Engine
 */

#include "Engine/Component/USpotLightComponent.h"

#include <algorithm>
#include <cmath>

namespace Engine {

    /**
     * @brief USpotLightComponent의 기본 상태를 초기화합니다.
     */
    Engine::USpotLightComponent::USpotLightComponent() = default;

    /**
     * @brief USpotLightComponent 객체의 수명을 끝내고 소유 자원을 정리합니다.
     */
    Engine::USpotLightComponent::~USpotLightComponent() = default;

    /**
     * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
     */
    Engine::FLightSceneProxy Engine::USpotLightComponent::CreateLightProxy() const {
        Engine::FLightSceneProxy Proxy{Engine::UPointLightComponent::CreateLightProxy()};
        Proxy.mType = Engine::ELightType::Spot;
        Proxy.mInnerConeAngle = mInnerConeAngle;
        Proxy.mOuterConeAngle = mOuterConeAngle;

        return Proxy;
    }

    /**
     * @brief 스포트라이트 내부 원뿔의 반각(도)를 반환합니다.
     */
    float Engine::USpotLightComponent::GetInnerConeAngle() const {
        return mInnerConeAngle;
    }

    /**
     * @brief 스포트라이트 내부 원뿔의 반각(도)를 검증하고 변경합니다.
     */
    bool Engine::USpotLightComponent::SetInnerConeAngle(float InnerConeAngle) {
        if (!std::isfinite(InnerConeAngle) || InnerConeAngle < 0.0f || InnerConeAngle >= 89.0f) {
            return false;
        }

        mInnerConeAngle = InnerConeAngle;
        mOuterConeAngle = std::max(mOuterConeAngle, mInnerConeAngle);

        SynchronizeLight();

        return true;
    }

    /**
     * @brief 스포트라이트 외부 원뿔의 반각(도)를 반환합니다.
     */
    float Engine::USpotLightComponent::GetOuterConeAngle() const {
        return mOuterConeAngle;
    }

    /**
     * @brief 스포트라이트 외부 원뿔의 반각(도)를 검증하고 변경합니다.
     */
    bool Engine::USpotLightComponent::SetOuterConeAngle(float OuterConeAngle) {
        if (!std::isfinite(OuterConeAngle) || OuterConeAngle < 0.0f || OuterConeAngle >= 89.0f) {
            return false;
        }

        mOuterConeAngle = OuterConeAngle;
        mInnerConeAngle = std::min(mInnerConeAngle, mOuterConeAngle);

        SynchronizeLight();

        return true;
    }

}
