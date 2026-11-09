/**
 * @file APointLightActor.h
 * @brief 점광원 컴포넌트를 기본으로 생성하는 액터입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/AActor.h"
#include "APointLightActor.generated.h"

namespace Engine {

    class UPointLightComponent;

    /**
     * @brief 점광원 컴포넌트를 기본으로 생성하는 액터입니다.
     * @ingroup Engine
     */
    UCLASS()
    class APointLightActor final : public Engine::AActor {
        GENERATED_BODY()

    public:
        /**
         * @brief APointLightActor의 기본 상태를 초기화합니다.
         */
        APointLightActor();

        /**
         * @brief APointLightActor 객체의 수명을 끝내고 소유 자원을 정리합니다.
         */
        ~APointLightActor() override;

    public:
        /**
         * @brief 액터 종류에 맞는 기본 광원 컴포넌트를 생성하고 루트에 연결합니다.
         * @return 새 컴포넌트를 연결하면 true이며 이미 컴포넌트가 있거나 연결에 실패하면 false입니다.
         */
        bool InitializeDefaultComponents() override;

        /**
         * @brief 액터의 기본 광원 컴포넌트를 조회합니다.
         * @return 액터가 소유한 광원 컴포넌트의 비소유 주소이며 없으면 nullptr입니다.
         */
        Engine::UPointLightComponent* GetLightComponent() const;

    };

}
