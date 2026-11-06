/**
 * @file UPointLightComponent.h
 * @brief 반경과 감쇠 지수를 갖는 점광원을 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/ULightComponent.h"
#include "UPointLightComponent.generated.h"

namespace Engine {

    /**
     * @brief 반경과 감쇠 지수를 갖는 점광원을 관리합니다.
     * @ingroup Engine
     */
    UCLASS()
    class UPointLightComponent : public Engine::ULightComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief UPointLightComponent의 기본 상태를 초기화합니다.
         */
        UPointLightComponent();

        /**
         * @brief UPointLightComponent 객체의 수명을 끝내고 소유 자원을 정리합니다.
         */
        ~UPointLightComponent() override;

    public:
        /**
         * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
         * @return 렌더 장면에 복사할 광원 상태와 아이콘의 비소유 주소입니다.
         */
        Engine::FLightSceneProxy CreateLightProxy() const override;

        /**
         * @brief 점광원·스포트라이트의 영향 반경을 반환합니다.
         * @return 점광원·스포트라이트의 영향 반경입니다.
         */
        float GetAttenuationRadius() const;

        /**
         * @brief 점광원·스포트라이트의 영향 반경을 검증하고 변경합니다.
         * @param[in] AttenuationRadius 0보다 큰 유한한 광원 영향 반경입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetAttenuationRadius(float AttenuationRadius);

        /**
         * @brief 거리에 따른 광원 감쇠 지수를 반환합니다.
         * @return 거리에 따른 광원 감쇠 지수입니다.
         */
        float GetLightFalloffExponent() const;

        /**
         * @brief 거리에 따른 광원 감쇠 지수를 검증하고 변경합니다.
         * @param[in] LightFalloffExponent 0 이상인 유한한 감쇠 지수입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetLightFalloffExponent(float LightFalloffExponent);

    private:
        /**
         * @brief 점광원·스포트라이트의 영향 반경입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetAttenuationRadius, Setter = SetAttenuationRadius)
        float mAttenuationRadius{10.0f};

        /**
         * @brief 거리에 따른 광원 감쇠 지수입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetLightFalloffExponent, Setter = SetLightFalloffExponent)
        float mLightFalloffExponent{2.0f};
    };

}
