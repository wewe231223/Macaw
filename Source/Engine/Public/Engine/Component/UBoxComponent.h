/**
 * @file UBoxComponent.h
 * @brief 로컬 반길이로 정의한 상자 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UShapeComponent.h"

namespace Engine {

    /**
     * @brief 로컬 반길이로 정의한 상자 충돌 컴포넌트입니다.
     */
    class UBoxComponent : public Engine::UShapeComponent {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(UBoxComponent, Engine::UShapeComponent);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
         */
        ReflectClass(UBoxComponent);

        /**
         * @brief 검사에 사용할 충돌 형상 값을 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Engine::FCollisionShape GetCollisionShape() const override;

        /**
         * @brief 충돌 형상의 로컬 축 정렬 경계를 계산합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::FBox CalcLocalBounds() const override;

        /**
         * @brief 상자의 로컬 축별 반길이를 설정합니다.
         * @param[in] InExtent 상자 각 축의 반길이입니다.
         */
        void SetBoxExtent(const Core::FVector& InExtent);

        /**
         * @brief 월드 스케일을 반영한 상자 축별 반길이를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::FVector GetScaledBoxExtent() const;

        /**
         * @brief 상자의 로컬 축별 반길이를 반환합니다.
         * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
         */
        const Core::FVector& GetBoxExtent() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 상자의 로컬 축별 반길이입니다.
         */
        Core::FVector mBoxExtent{0.5f, 0.5f, 0.5f};
        ReflectAccessor(mBoxExtent, GetBoxExtent, SetBoxExtent, "BoxExtent", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);
    };

}
