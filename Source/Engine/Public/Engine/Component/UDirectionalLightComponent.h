/**
 * @file UDirectionalLightComponent.h
 * @brief 방향광과 캐스케이드 그림자 설정을 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/ULightComponent.h"

namespace Engine {

    /**
     * @brief 방향광과 캐스케이드 그림자 설정을 관리합니다.
     * @ingroup Engine
     */
    class UDirectionalLightComponent final : public Engine::ULightComponent {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(UDirectionalLightComponent, Engine::ULightComponent);

    public:
        /**
         * @brief UDirectionalLightComponent의 기본 상태를 초기화합니다.
         */
        UDirectionalLightComponent();

        /**
         * @brief UDirectionalLightComponent 객체의 수명을 끝내고 소유 자원을 정리합니다.
         */
        ~UDirectionalLightComponent() override;

    public:
        /**
         * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
         * @return 등록된 클래스 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(UDirectionalLightComponent);

        /**
         * @brief 방향광 그림자의 캐스케이드 개수를 반환합니다.
         * @return 방향광 그림자의 캐스케이드 개수입니다.
         */
        Core::Uint32 GetCascadeCount() const;

        /**
         * @brief 방향광 그림자의 캐스케이드 개수를 검증하고 변경합니다.
         * @param[in] Count 1 이상 4 이하인 캐스케이드 개수입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetCascadeCount(Core::Uint32 Count);

        /**
         * @brief 방향광 그림자의 최대 표시 거리를 반환합니다.
         * @return 방향광 그림자의 최대 표시 거리입니다.
         */
        float GetShadowDistance() const;

        /**
         * @brief 방향광 그림자의 최대 표시 거리를 검증하고 변경합니다.
         * @param[in] Distance 1 이상 100000 이하인 유한한 그림자 표시 거리입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetShadowDistance(float Distance);

        /**
         * @brief 각 캐스케이드 구간에서 전환에 사용하는 비율을 반환합니다.
         * @return 각 캐스케이드 구간에서 전환에 사용하는 비율입니다.
         */
        float GetCascadeTransitionFraction() const;

        /**
         * @brief 각 캐스케이드 구간에서 전환에 사용하는 비율을 검증하고 변경합니다.
         * @param[in] Fraction 0 이상 0.5 이하인 유한한 캐스케이드 전환 비율입니다.
         * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
         */
        bool SetCascadeTransitionFraction(float Fraction);

        /**
         * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
         * @return 렌더 장면에 복사할 광원 상태와 아이콘의 비소유 주소입니다.
         */
        Engine::FLightSceneProxy CreateLightProxy() const override;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 방향광 그림자의 캐스케이드 개수입니다.
         */
        Core::Uint32 mCascadeCount{4};
        ReflectAccessor(mCascadeCount, GetCascadeCount, SetCascadeCount, "CascadeCount", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 방향광 그림자의 최대 표시 거리입니다.
         */
        float mShadowDistance{100.0f};
        ReflectAccessor(mShadowDistance, GetShadowDistance, SetShadowDistance, "ShadowDistance", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 각 캐스케이드 구간에서 전환에 사용하는 비율입니다.
         */
        float mCascadeTransitionFraction{0.1f};
        ReflectAccessor(mCascadeTransitionFraction, GetCascadeTransitionFraction, SetCascadeTransitionFraction, "CascadeTransitionFraction", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);
    };

}
