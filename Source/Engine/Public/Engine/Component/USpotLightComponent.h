/**
 * @file USpotLightComponent.h
 * @brief 내부·외부 원뿔각으로 제한되는 스포트라이트를 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UPointLightComponent.h"
#include "USpotLightComponent.generated.h"

namespace Engine {

    /**
     * @brief 내부·외부 원뿔각으로 제한되는 스포트라이트를 관리합니다.
     * @ingroup Engine
     */
    UCLASS()
    class USpotLightComponent final : public Engine::UPointLightComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief USpotLightComponent의 기본 상태를 초기화합니다.
         */
        USpotLightComponent();

        /**
         * @brief USpotLightComponent 객체의 수명을 끝내고 소유 자원을 정리합니다.
         */
        ~USpotLightComponent() override;

    public:
        /**
         * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
         * @return 렌더 장면에 복사할 광원 상태와 아이콘의 비소유 주소입니다.
         */
        Engine::FLightSceneProxy CreateLightProxy() const override;

        /**
         * @brief 스포트라이트 내부 원뿔의 반각(도)를 반환합니다.
         * @return 스포트라이트 내부 원뿔의 반각(도)입니다.
         */
        float GetInnerConeAngle() const;

        /**
         * @brief 스포트라이트 내부 원뿔의 반각(도)를 검증하고 변경합니다.
         * @param[in] InnerConeAngle 0 이상 89 미만인 유한한 내부 반각(도)입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         * @note 외부 원뿔각이 더 작으면 외부 각도도 함께 늘립니다.
         */
        bool SetInnerConeAngle(float InnerConeAngle);

        /**
         * @brief 스포트라이트 외부 원뿔의 반각(도)를 반환합니다.
         * @return 스포트라이트 외부 원뿔의 반각(도)입니다.
         */
        float GetOuterConeAngle() const;

        /**
         * @brief 스포트라이트 외부 원뿔의 반각(도)를 검증하고 변경합니다.
         * @param[in] OuterConeAngle 0 이상 89 미만인 유한한 외부 반각(도)입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         * @note 내부 원뿔각이 더 크면 내부 각도도 함께 줄입니다.
         */
        bool SetOuterConeAngle(float OuterConeAngle);

    private:
        /**
         * @brief 스포트라이트 내부 원뿔의 반각(도)입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetInnerConeAngle, Setter = SetInnerConeAngle)
        float mInnerConeAngle{20.0f};

        /**
         * @brief 스포트라이트 외부 원뿔의 반각(도)입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetOuterConeAngle, Setter = SetOuterConeAngle)
        float mOuterConeAngle{35.0f};
    };

}
