/**
 * @file AStaticMeshActor.h
 * @brief 기본 UStaticMeshComponent를 생성하는 프리미티브 액터입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/AActor.h"
#include "AStaticMeshActor.generated.h"

namespace Engine {

    class UStaticMeshComponent;

    /**
     * @brief 기본 UStaticMeshComponent를 생성하는 프리미티브 액터입니다.
     * @ingroup Engine
     */
    UCLASS()
    class AStaticMeshActor final : public Engine::AActor {
        GENERATED_BODY()

    public:
        /**
         * @brief AStaticMeshActor의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        AStaticMeshActor();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~AStaticMeshActor() override;

    public:
        /**
         * @brief 액터의 기본 컴포넌트를 생성하고 루트 컴포넌트를 정합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool InitializeDefaultComponents() override;

        /**
         * @brief 액터의 기본 메시 컴포넌트 비소유 주소를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        Engine::UStaticMeshComponent* GetStaticMeshComponent() const;

    };

}
