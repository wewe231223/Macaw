/**
 * @file ULightComponent.h
 * @brief 광원의 그림자 생성 여부와 해상도·바이어스 설정을 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/ULightComponentBase.h"
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
         * @brief 그림자 깊이 비교의 상수 바이어스를 텍셀 단위로 반환합니다.
         * @return 그림자 맵 텍셀 단위의 상수 바이어스입니다.
         */
        float GetShadowBias() const;

        /**
         * @brief 그림자 깊이 비교의 상수 바이어스를 검증하고 변경합니다.
         * @param[in] Bias 0 이상 16 이하인 유한한 텍셀 단위 바이어스입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetShadowBias(float Bias);

        /**
         * @brief 표면 기울기에 따른 그림자 바이어스 배율을 반환합니다.
         * @return 표면 기울기에 따른 그림자 바이어스 배율입니다.
         */
        float GetShadowSlopeBias() const;

        /**
         * @brief 표면 기울기에 따른 그림자 바이어스 배율을 검증하고 변경합니다.
         * @param[in] Bias 0 이상 16 이하인 유한한 기울기 바이어스입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetShadowSlopeBias(float Bias);

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
         * @brief 그림자 깊이 비교의 상수 바이어스이며 단위는 그림자 맵 텍셀입니다. 값의 의미는 캐스케이드와 맵의 깊이 범위에 좌우되지 않습니다.
         */
        UPROPERTY(Edit, Serialize, Name = "ShadowBiasTexels", Getter = GetShadowBias, Setter = SetShadowBias)
        float mShadowBias{2.0f};

        /**
         * @brief 표면 기울기에 따른 그림자 바이어스 배율입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetShadowSlopeBias, Setter = SetShadowSlopeBias)
        float mShadowSlopeBias{1.0f};

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
