/**
 * @file ADecalActor.h
 * @brief 데칼 컴포넌트를 루트로 소유하는 장면 액터입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/AActor.h"
#include "ADecalActor.generated.h"

namespace Engine {

    class UDecalComponent;

    /**
     * @brief 데칼 컴포넌트를 루트로 소유하는 장면 액터입니다.
     * @ingroup Engine
     */
    UCLASS()
    class ADecalActor final : public Engine::AActor {
        GENERATED_BODY()

    public:
        bool InitializeDefaultComponents() override;

        /**
         * @brief 액터가 소유한 루트 데칼 컴포넌트를 반환합니다.
         * @return 액터 수명에 종속되는 비소유 컴포넌트 포인터입니다.
         */
        Engine::UDecalComponent* GetDecalComponent() const;
    };

}
