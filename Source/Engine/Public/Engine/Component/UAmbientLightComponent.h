/**
 * @file UAmbientLightComponent.h
 * @brief 위치와 방향에 무관한 환경광을 렌더 장면에 전달합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/ULightComponent.h"
#include "UAmbientLightComponent.generated.h"

namespace Engine {

    /**
     * @brief 위치와 방향에 무관한 환경광을 렌더 장면에 전달합니다.
     * @ingroup Engine
     */
    UCLASS()
    class UAmbientLightComponent final : public Engine::ULightComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief UAmbientLightComponent의 기본 상태를 초기화합니다.
         */
        UAmbientLightComponent();

        /**
         * @brief UAmbientLightComponent 객체의 수명을 끝내고 소유 자원을 정리합니다.
         */
        ~UAmbientLightComponent() override;

    public:
        /**
         * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
         * @return 렌더 장면에 복사할 광원 상태와 아이콘의 비소유 주소입니다.
         */
        Engine::FLightSceneProxy CreateLightProxy() const override;

    };

}
