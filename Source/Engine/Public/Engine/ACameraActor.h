/**
 * @file ACameraActor.h
 * @brief 카메라 컴포넌트를 갖고 이동·회전 입력을 처리하는 액터에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/AActor.h"
#include "ACameraActor.generated.h"

namespace Engine {

    class UCameraComponent;

    /**
     * @brief 카메라 컴포넌트를 갖고 이동·회전 입력을 처리하는 액터를 담당합니다.
     */
    UCLASS()
    class ACameraActor : public Engine::AActor {
        GENERATED_BODY()

    public:
        /**
         * @brief 액터의 기본 컴포넌트를 생성하고 루트 컴포넌트를 정합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool InitializeDefaultComponents() override;

        /**
         * @brief 이 액터에 소속된 카메라 컴포넌트를 조회합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Engine::UCameraComponent* GetCameraComponent() const;

        /**
         * @brief 뷰포트 입력을 받아 프레임 시간에 따라 액터의 동작을 갱신합니다.
         * @param[in] Input 현재 프레임의 키보드·마우스 입력 상태입니다.
         * @param[in] DeltaTime 이전 갱신 이후의 경과 시간이며 초 단위입니다.
         */
        void Input(const Core::FInput& Input, float DeltaTime) override;

    };

}
