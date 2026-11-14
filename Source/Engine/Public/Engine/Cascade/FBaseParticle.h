/**
 * @file FBaseParticle.h
 * @brief 모듈들이 공유하는 파티클 기본 속성의 저장소와 접근 함수를 선언합니다.
 * @ingroup Cascade
 */

#pragma once

#include "Core/Math/FVector.h"
#include "Core/Math/FVector4.h"

/**
 * @brief 파티클 시스템 정의와 모듈 실행 컨텍스트 및 인스턴스 저장소를 제공합니다.
 */
namespace Engine {

    /**
     * @brief 모든 파티클이 공통으로 사용하는 위치·속도·크기·색상·수명 데이터를 보관합니다.
     * @details 모듈 전용 추가 데이터는 이 기본 데이터 뒤의 별도 페이로드 영역에 배치합니다.
     * @ingroup Cascade
     */
    struct FBaseParticle {
    public:
        /**
         * @brief 위치·속도·수명·회전은 0, 크기와 RGBA 색상 성분은 1인 기본 파티클을 구성합니다.
         */
        FBaseParticle();

    public:
        /**
         * @brief 파티클의 현재 위치 속성을 직접 수정할 수 있도록 반환합니다.
         * @return 이 파티클이 보관하는 값의 수정 가능한 참조입니다.
         */
        Core::FVector& GetLocation();
        /**
         * @brief 파티클의 현재 위치 속성을 읽습니다.
         * @return 이 파티클이 보관하는 값의 읽기 전용 참조입니다.
         */
        const Core::FVector& GetLocation() const;
        /**
         * @brief 파티클의 현재 속도 속성을 직접 수정할 수 있도록 반환합니다.
         * @return 이 파티클이 보관하는 값의 수정 가능한 참조입니다.
         */
        Core::FVector& GetVelocity();
        /**
         * @brief 파티클의 현재 속도 속성을 읽습니다.
         * @return 이 파티클이 보관하는 값의 읽기 전용 참조입니다.
         */
        const Core::FVector& GetVelocity() const;
        /**
         * @brief 파티클의 축별 크기 속성을 직접 수정할 수 있도록 반환합니다.
         * @return 이 파티클이 보관하는 값의 수정 가능한 참조입니다.
         */
        Core::FVector& GetSize();
        /**
         * @brief 파티클의 축별 크기 속성을 읽습니다.
         * @return 이 파티클이 보관하는 값의 읽기 전용 참조입니다.
         */
        const Core::FVector& GetSize() const;
        /**
         * @brief 파티클의 알파를 포함한 RGBA 색상 속성을 직접 수정할 수 있도록 반환합니다.
         * @return 이 파티클이 보관하는 값의 수정 가능한 참조입니다.
         */
        Core::FVector4& GetColor();
        /**
         * @brief 파티클의 알파를 포함한 RGBA 색상 속성을 읽습니다.
         * @return 이 파티클이 보관하는 값의 읽기 전용 참조입니다.
         */
        const Core::FVector4& GetColor() const;

        /**
         * @brief 파티클의 수명 대비 정규화된 경과 시간 속성을 직접 수정할 수 있도록 반환합니다.
         * @return 이 파티클이 보관하는 값의 수정 가능한 참조입니다.
         */
        float& GetRelativeTime();
        /**
         * @brief 파티클의 수명 대비 정규화된 경과 시간 속성을 읽습니다.
         * @return 현재 저장된 스칼라 값입니다.
         */
        float GetRelativeTime() const;
        /**
         * @brief 파티클의 최대 수명의 역수 속성을 직접 수정할 수 있도록 반환합니다.
         * @return 이 파티클이 보관하는 값의 수정 가능한 참조입니다.
         */
        float& GetOneOverMaxLifetime();
        /**
         * @brief 파티클의 최대 수명의 역수 속성을 읽습니다.
         * @return 현재 저장된 스칼라 값입니다.
         */
        float GetOneOverMaxLifetime() const;
        /**
         * @brief 파티클의 현재 회전값 속성을 직접 수정할 수 있도록 반환합니다.
         * @return 이 파티클이 보관하는 값의 수정 가능한 참조입니다.
         */
        float& GetRotation();
        /**
         * @brief 파티클의 현재 회전값 속성을 읽습니다.
         * @return 현재 저장된 스칼라 값입니다.
         */
        float GetRotation() const;
        /**
         * @brief 파티클의 현재 회전 속도 속성을 직접 수정할 수 있도록 반환합니다.
         * @return 이 파티클이 보관하는 값의 수정 가능한 참조입니다.
         */
        float& GetRotationRate();
        /**
         * @brief 파티클의 현재 회전 속도 속성을 읽습니다.
         * @return 현재 저장된 스칼라 값입니다.
         */
        float GetRotationRate() const;

    private:
        /**
         * @brief 파티클의 현재 위치입니다.
         */
        Core::FVector mLocation{};
        /**
         * @brief 파티클의 현재 속도입니다.
         */
        Core::FVector mVelocity{};
        /**
         * @brief 파티클의 축별 크기입니다.
         */
        Core::FVector mSize{1.0f, 1.0f, 1.0f};
        /**
         * @brief 파티클의 알파를 포함한 RGBA 색상입니다.
         */
        Core::FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
        /**
         * @brief 파티클의 수명 대비 정규화된 경과 시간입니다.
         */
        float mRelativeTime{};
        /**
         * @brief 파티클의 최대 수명의 역수입니다.
         */
        float mOneOverMaxLifetime{};
        /**
         * @brief 파티클의 현재 회전값입니다.
         */
        float mRotation{};
        /**
         * @brief 파티클의 현재 회전 속도입니다.
         */
        float mRotationRate{};
    };

}
