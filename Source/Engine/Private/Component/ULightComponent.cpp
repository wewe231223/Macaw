/**
 * @file ULightComponent.cpp
 * @brief 광원의 그림자 생성 여부와 해상도·바이어스 설정을 관리합니다.
 * @ingroup Engine
 */

#include "Engine/Component/ULightComponent.h"
#include "Core/Archive.h"

#include <cmath>

namespace Engine {

    /**
     * @brief ULightComponent의 기본 상태를 초기화합니다.
     */
    Engine::ULightComponent::ULightComponent() = default;

    /**
     * @brief ULightComponent 객체의 수명을 끝내고 소유 자원을 정리합니다.
     */
    Engine::ULightComponent::~ULightComponent() = default;

    /**
     * @brief 광원의 그림자 생성 여부를 반환합니다.
     */
    bool Engine::ULightComponent::GetCastShadows() const {
        return mCastShadows;
    }

    /**
     * @brief 광원의 그림자 생성 여부를 검증하고 변경합니다.
     */
    bool Engine::ULightComponent::SetCastShadows(bool CastShadows) {
        mCastShadows = CastShadows;

        SynchronizeLight();

        return true;
    }

    /**
     * @brief 기본 그림자 맵의 한 변 해상도를 반환합니다.
     */
    Core::Uint32 Engine::ULightComponent::GetShadowResolution() const {
        return mShadowResolution;
    }

    /**
     * @brief 기본 그림자 맵의 한 변 해상도를 검증하고 변경합니다.
     */
    bool Engine::ULightComponent::SetShadowResolution(Core::Uint32 Resolution) {
        if (Resolution < 64 || Resolution > 8192 || (Resolution & (Resolution - 1)) != 0) {
            return false;
        }

        mShadowResolution = Resolution;

        SynchronizeLight();

        return true;
    }

    /**
     * @brief 그림자 해상도 배율을 반환합니다.
     */
    float Engine::ULightComponent::GetShadowResolutionScale() const {
        return mShadowResolutionScale;
    }

    /**
     * @brief 그림자 해상도 배율을 검증하고 변경합니다.
     */
    bool Engine::ULightComponent::SetShadowResolutionScale(float Scale) {
        if (!std::isfinite(Scale) || Scale < 0.125f || Scale > 8.0f) {
            return false;
        }

        mShadowResolutionScale = Scale;

        SynchronizeLight();

        return true;
    }

    Engine::EShadowBiasPreset Engine::ULightComponent::GetShadowBiasPreset() const {
        return mShadowBiasPreset;
    }

    bool Engine::ULightComponent::SetShadowBiasPreset(Engine::EShadowBiasPreset Preset) {
        if (static_cast<Core::Uint32>(Preset) >= Engine::ShadowBiasPresetCount) {
            return false;
        }

        mShadowBiasPreset = Preset;

        SynchronizeLight();

        return true;
    }

    Engine::EShadowBiasPreset Engine::ULightComponent::GetShadowSlopeBiasPreset() const {
        return mShadowSlopeBiasPreset;
    }

    bool Engine::ULightComponent::SetShadowSlopeBiasPreset(Engine::EShadowBiasPreset Preset) {
        if (static_cast<Core::Uint32>(Preset) >= Engine::ShadowBiasPresetCount) {
            return false;
        }

        mShadowSlopeBiasPreset = Preset;

        SynchronizeLight();

        return true;
    }

    /**
     * @brief 그림자 경계의 선명도 조절값을 반환합니다.
     */
    float Engine::ULightComponent::GetShadowSharpen() const {
        return mShadowSharpen;
    }

    /**
     * @brief 그림자 경계의 선명도 조절값을 검증하고 변경합니다.
     */
    bool Engine::ULightComponent::SetShadowSharpen(float Sharpen) {
        if (!std::isfinite(Sharpen) || Sharpen < 0.0f || Sharpen > 10.0f) {
            return false;
        }

        mShadowSharpen = Sharpen;

        SynchronizeLight();

        return true;
    }

    /**
     * @brief PCSS 반그림자 계산에 쓰는 광원 반지름을 반환합니다.
     */
    float Engine::ULightComponent::GetShadowSourceRadius() const {
        return mShadowSourceRadius;
    }

    /**
     * @brief PCSS 반그림자 계산에 쓰는 광원 반지름을 검증하고 변경합니다.
     */
    bool Engine::ULightComponent::SetShadowSourceRadius(float Radius) {
        if (!std::isfinite(Radius) || Radius < 0.0f || Radius > 10.0f) {
            return false;
        }

        mShadowSourceRadius = Radius;

        SynchronizeLight();

        return true;
    }

    /**
     * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
     */
    Engine::FLightSceneProxy Engine::ULightComponent::CreateLightProxy() const {
        Engine::FLightSceneProxy Proxy{Engine::ULightComponentBase::CreateLightProxy()};
        Proxy.mCastShadows = mCastShadows;
        Proxy.mShadowResolution = mShadowResolution;
        Proxy.mShadowResolutionScale = mShadowResolutionScale;
        Proxy.mShadowBiasPreset = mShadowBiasPreset;
        Proxy.mShadowSlopeBiasPreset = mShadowSlopeBiasPreset;
        Proxy.mShadowSharpen = mShadowSharpen;
        Proxy.mShadowSourceRadius = mShadowSourceRadius;

        return Proxy;
    }

    void Engine::ULightComponent::Serialize(Core::FArchive& Archive) {
        Engine::ULightComponentBase::Serialize(Archive);

        if (Archive.IsLoading() && !Archive.HasError() && Archive.HasField("ShadowBiasPreset") && !Archive.HasField("ShadowSlopeBiasPreset")) {
            SetShadowSlopeBiasPreset(mShadowBiasPreset);
        }
    }

}
