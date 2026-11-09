/**
 * @file USpriteComponent.h
 * @brief 로컬 YZ 평면의 고정 텍스처 스프라이트를 제공합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UBillboardComponent.h"
#include "USpriteComponent.generated.h"

namespace Engine {

    /**
     * @brief 카메라 방향과 독립적으로 로컬 YZ 평면에 표시하는 텍스처 스프라이트입니다.
     */
    UCLASS()
    class USpriteComponent final : public Engine::UBillboardComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief 카메라 정렬을 끈 고정 평면 스프라이트를 생성합니다.
         */
        USpriteComponent();

        /**
         * @brief 기본 소멸자이며, 렌더 장면 등록 해제는 UPrimitiveComponent 소멸자가 처리합니다.
         */
        ~USpriteComponent() override;

    };

}
