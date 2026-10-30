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
         * @brief 객체를 소멸하고 소유한 GPU 자원 또는 UV 제어 상태를 해제합니다.
         */
        ~USpriteComponent() override;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
         */
        ReflectClass(USpriteComponent);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();
    };

}
