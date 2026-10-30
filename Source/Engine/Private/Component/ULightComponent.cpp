/**
 * @file ULightComponent.cpp
 * @brief 광원의 그림자 생성 여부와 해상도·바이어스 설정을 관리합니다.
 * @ingroup Engine
 */

#include "Engine/Component/ULightComponent.h"

#include <cmath>

namespace Engine {

    Engine::ULightComponent::ULightComponent() = default;
    Engine::ULightComponent::~ULightComponent() = default;

    bool Engine::ULightComponent::GetCastShadows() const {
        return mCastShadows;
    }

    bool Engine::ULightComponent::SetCastShadows(bool CastShadows) {
        mCastShadows = CastShadows;
        SynchronizeLight();
        return true;
    }

    Core::Uint32 Engine::ULightComponent::GetShadowResolution() const {
        return mShadowResolution;
    }

    bool Engine::ULightComponent::SetShadowResolution(Core::Uint32 Resolution) {
        if (Resolution < 64 || Resolution > 8192 || (Resolution & (Resolution - 1)) != 0) {
            return false;
        }

        mShadowResolution = Resolution;
        SynchronizeLight();

        return true;
    }

    float Engine::ULightComponent::GetShadowResolutionScale() const {
        return mShadowResolutionScale;
    }

    bool Engine::ULightComponent::SetShadowResolutionScale(float Scale) {
        if (!std::isfinite(Scale) || Scale < 0.125f || Scale > 8.0f) {
            return false;
        }

        mShadowResolutionScale = Scale;
        SynchronizeLight();

        return true;
    }

    float Engine::ULightComponent::GetShadowBias() const {
        return mShadowBias;
    }

    bool Engine::ULightComponent::SetShadowBias(float Bias) {
        if (!std::isfinite(Bias) || Bias < 0.0f || Bias > 0.1f) {
            return false;
        }

        mShadowBias = Bias;
        SynchronizeLight();

        return true;
    }

    float Engine::ULightComponent::GetShadowSlopeBias() const {
        return mShadowSlopeBias;
    }

    bool Engine::ULightComponent::SetShadowSlopeBias(float Bias) {
        if (!std::isfinite(Bias) || Bias < 0.0f || Bias > 16.0f) {
            return false;
        }

        mShadowSlopeBias = Bias;
        SynchronizeLight();

        return true;
    }

    float Engine::ULightComponent::GetShadowSharpen() const {
        return mShadowSharpen;
    }

    bool Engine::ULightComponent::SetShadowSharpen(float Sharpen) {
        if (!std::isfinite(Sharpen) || Sharpen < 0.0f || Sharpen > 10.0f) {
            return false;
        }

        mShadowSharpen = Sharpen;
        SynchronizeLight();

        return true;
    }

    Engine::FLightSceneProxy Engine::ULightComponent::CreateLightProxy() const {
        Engine::FLightSceneProxy Proxy{Engine::ULightComponentBase::CreateLightProxy()};
        Proxy.mCastShadows = mCastShadows;
        Proxy.mShadowResolution = mShadowResolution;
        Proxy.mShadowResolutionScale = mShadowResolutionScale;
        Proxy.mShadowBias = mShadowBias;
        Proxy.mShadowSlopeBias = mShadowSlopeBias;
        Proxy.mShadowSharpen = mShadowSharpen;

        return Proxy;
    }

}
