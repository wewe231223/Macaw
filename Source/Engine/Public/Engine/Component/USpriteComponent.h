/**
 * @file USpriteComponent.h
 * @brief 로컬 YZ 평면의 고정 텍스처 스프라이트를 제공합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UBillboardComponent.h"

namespace Engine {

    /**
     * @brief 카메라 방향과 독립적으로 로컬 YZ 평면에 표시하는 텍스처 스프라이트입니다.
     */
    class USpriteComponent final : public Engine::UBillboardComponent {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(USpriteComponent, Engine::UBillboardComponent);

    public:
        /**
         * @brief 카메라 정렬을 끈 고정 평면 스프라이트를 생성합니다.
         */
        USpriteComponent();

        /**
         * @brief 기본 소멸자이며, 렌더 장면 등록 해제는 UPrimitiveComponent 소멸자가 처리합니다.
         */
        ~USpriteComponent() override;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(USpriteComponent);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();
    };

}
