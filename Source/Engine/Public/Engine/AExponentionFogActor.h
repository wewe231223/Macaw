/**
 * @file AExponentionFogActor.h
 * @brief 높이 기반 지수 안개 컴포넌트를 루트로 생성하여 에디터에서 배치하는 액터입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/AActor.h"
#include "AExponentionFogActor.generated.h"

namespace Engine {

    class UHeightFogComponent;

    /**
     * @brief 높이 기반 지수 안개 컴포넌트를 루트로 생성하여 에디터에서 배치하는 액터입니다.
     */
    UCLASS()
    class AExponentionFogActor final : public Engine::AActor {
        GENERATED_BODY()

    public:
        /**
         * @brief 기본 설정으로 AExponentionFogActor 객체를 초기화합니다.
         */
        AExponentionFogActor();

        /**
         * @brief 객체 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~AExponentionFogActor() override;

    public:
        /**
         * @brief 빈 액터에 HeightFogComponent를 생성하고 루트 컴포넌트로 추가합니다.
         * @return 컴포넌트를 추가하면 true이며 이미 컴포넌트가 있거나 추가에 실패하면 false입니다.
         */
        bool InitializeDefaultComponents() override;

        /**
         * @brief 안개 액터의 루트에 연결된 높이 안개 컴포넌트를 조회합니다.
         * @return 루트가 높이 안개 컴포넌트이면 비소유 주소를 반환하며 그렇지 않으면 nullptr입니다.
         */
        Engine::UHeightFogComponent* GetFogComponent() const;

    };

}
