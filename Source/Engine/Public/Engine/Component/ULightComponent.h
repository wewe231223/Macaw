/**
 * @file ULightComponent.h
 * @brief 광원의 그림자 생성 여부와 해상도·바이어스 설정을 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/ULightComponentBase.h"
#include "Engine/EShadowBiasPreset.h"
#include "ULightComponent.generated.h"

namespace Engine {

    /**
     * @brief 광원의 그림자 생성 여부와 해상도·바이어스 설정을 관리합니다.
     * @ingroup Engine
     */
    UCLASS(NoFactory)
    class ULightComponent : public Engine::ULightComponentBase {
        GENERATED_BODY()

    public:

        /**
         * @brief ULightComponent의 기본 상태를 초기화합니다.
         */
        ULightComponent();

        /**
         * @brief ULightComponent 객체의 수명을 끝내고 소유 자원을 정리합니다.
         */
        ~ULightComponent() override;

    public:

        /**
         * @brief 광원의 그림자 생성 여부를 반환합니다.
         * @return 광원의 그림자 생성 여부입니다.
         */
        bool GetCastShadows() const;

        /**
         * @brief 광원의 그림자 생성 여부를 검증하고 변경합니다.
         * @param[in] CastShadows 광원이 그림자를 생성하도록 설정하려면 true입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetCastShadows(bool CastShadows);

        /**
         * @brief 기본 그림자 맵의 한 변 해상도를 반환합니다.
         * @return 기본 그림자 맵의 한 변 해상도입니다.
         */
        Core::Uint32 GetShadowResolution() const;

        /**
         * @brief 기본 그림자 맵의 한 변 해상도를 검증하고 변경합니다.
         * @param[in] Resolution 64 이상 8192 이하인 2의 거듭제곱 해상도입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetShadowResolution(Core::Uint32 Resolution);

        /**
         * @brief 그림자 해상도 배율을 반환합니다.
         * @return 그림자 해상도 배율입니다.
         */
        float GetShadowResolutionScale() const;

        /**
         * @brief 그림자 해상도 배율을 검증하고 변경합니다.
         * @param[in] Scale 0.125 이상 8 이하인 유한한 해상도 배율입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetShadowResolutionScale(float Scale);

        /**
         * @brief 그림자 깊이 바이어스 프리셋을 반환합니다.
         * @return 현재 깊이 바이어스 단계입니다.
         */
        Engine::EShadowBiasPreset GetShadowBiasPreset() const;

        /**
         * @brief 유효한 깊이 바이어스 단계를 설정하고 조명 프록시를 갱신합니다.
         * @param Preset 유효 범위 안의 그림자 바이어스 단계입니다.
         * @return 프리셋 값이 유효하면 true입니다.
         */
        bool SetShadowBiasPreset(Engine::EShadowBiasPreset Preset);

        /**
         * @brief 그림자 경사 바이어스 프리셋을 반환합니다.
         * @return 현재 경사 바이어스 단계입니다.
         */
        Engine::EShadowBiasPreset GetShadowSlopeBiasPreset() const;

        /**
         * @brief 유효한 경사 바이어스 단계를 설정하고 조명 프록시를 갱신합니다.
         * @param Preset 유효 범위 안의 그림자 바이어스 단계입니다.
         * @return 프리셋 값이 유효하면 true입니다.
         */
        bool SetShadowSlopeBiasPreset(Engine::EShadowBiasPreset Preset);

        /**
         * @brief 그림자 경계의 선명도 조절값을 반환합니다.
         * @return 그림자 경계의 선명도 조절값입니다.
         */
        float GetShadowSharpen() const;

        /**
         * @brief 그림자 경계의 선명도 조절값을 검증하고 변경합니다.
         * @param[in] Sharpen 0 이상 10 이하인 유한한 경계 선명도입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetShadowSharpen(float Sharpen);

        /**
         * @brief PCSS 반그림자 계산에 쓰는 광원 반지름을 반환합니다.
         * @return 점광원·스포트라이트 광원의 반지름(m)입니다.
         */
        float GetShadowSourceRadius() const;

        /**
         * @brief PCSS 반그림자 계산에 쓰는 광원 반지름을 검증하고 변경합니다.
         * @param[in] Radius 0 이상 10 이하인 유한한 광원 반지름(m)입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetShadowSourceRadius(float Radius);

        /**
         * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
         * @return 렌더 장면에 복사할 광원 상태와 아이콘의 비소유 주소입니다.
         */
        Engine::FLightSceneProxy CreateLightProxy() const override;
        void Serialize(Core::FArchive& Archive) override;

    private:

        /**
         * @brief 광원의 그림자 생성 여부입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetCastShadows, Setter = SetCastShadows)
        bool mCastShadows{true};

        /**
         * @brief 기본 그림자 맵의 한 변 해상도입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetShadowResolution, Setter = SetShadowResolution)
        Core::Uint32 mShadowResolution{1024};

        /**
         * @brief 그림자 해상도 배율입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetShadowResolutionScale, Setter = SetShadowResolutionScale)
        float mShadowResolutionScale{1.0f};

        /**
         * @brief 그림자 깊이 바이어스 단계입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetShadowBiasPreset, Setter = SetShadowBiasPreset)
        Engine::EShadowBiasPreset mShadowBiasPreset{Engine::DefaultShadowBiasPreset};

        /**
         * @brief 그림자 경사 바이어스 단계입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetShadowSlopeBiasPreset, Setter = SetShadowSlopeBiasPreset)
        Engine::EShadowBiasPreset mShadowSlopeBiasPreset{Engine::DefaultShadowBiasPreset};

        /**
         * @brief 그림자 경계의 선명도 조절값입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetShadowSharpen, Setter = SetShadowSharpen)
        float mShadowSharpen{};

        /**
         * @brief PCSS 반그림자 계산에 쓰는 점광원·스포트라이트 광원의 반지름(m)입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetShadowSourceRadius, Setter = SetShadowSourceRadius)
        float mShadowSourceRadius{0.1f};
    };

}
