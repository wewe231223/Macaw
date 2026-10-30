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

    Engine::FLightSceneProxy Engine::UDirectionalLightComponent::CreateLightProxy() const {
        Engine::FLightSceneProxy Proxy{Engine::ULightComponent::CreateLightProxy()};
        Proxy.mType = Engine::ELightType::Directional;
        Proxy.mCascadeCount = mCascadeCount;
        Proxy.mShadowDistance = mShadowDistance;
        Proxy.mCascadeTransitionFraction = mCascadeTransitionFraction;

        return Proxy;
    }

    Core::Uint32 Engine::UDirectionalLightComponent::GetCascadeCount() const {
        return mCascadeCount;
    }

    bool Engine::UDirectionalLightComponent::SetCascadeCount(Core::Uint32 Count) {
        if (Count < 1 || Count > 4) {
            return false;
        }

        mCascadeCount = Count;
        SynchronizeLight();

        return true;
    }

    float Engine::UDirectionalLightComponent::GetShadowDistance() const {
        return mShadowDistance;
    }

    bool Engine::UDirectionalLightComponent::SetShadowDistance(float Distance) {
        if (!std::isfinite(Distance) || Distance < 1.0f || Distance > 100000.0f) {
            return false;
        }

        mShadowDistance = Distance;
        SynchronizeLight();

        return true;
    }

    float Engine::UDirectionalLightComponent::GetCascadeTransitionFraction() const {
        return mCascadeTransitionFraction;
    }

    bool Engine::UDirectionalLightComponent::SetCascadeTransitionFraction(float Fraction) {
        if (!std::isfinite(Fraction) || Fraction < 0.0f || Fraction > 0.5f) {
            return false;
        }

        mCascadeTransitionFraction = Fraction;
        SynchronizeLight();

        return true;
    }

}
