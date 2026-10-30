/**
 * @file UAmbientLightComponent.h
 * @brief 위치와 방향에 무관한 환경광을 렌더 장면에 전달합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/ULightComponent.h"

/**
 * @brief 위치와 방향에 무관한 환경광을 렌더 장면에 전달합니다.
 * @ingroup Engine
 */
class UAmbientLightComponent final : public ULightComponent {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UAmbientLightComponent, ULightComponent);

public:
    /**
     * @brief UAmbientLightComponent의 기본 상태를 초기화합니다.
     */
    UAmbientLightComponent();
    /**
     * @brief UAmbientLightComponent 객체의 수명을 끝내고 소유 자원을 정리합니다.
     */
    ~UAmbientLightComponent() override;

public:
    /**
     * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 클래스 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(UAmbientLightComponent);

    /**
     * @brief 현재 컴포넌트의 조명·그림자 설정으로 렌더 프록시를 생성합니다.
     * @return 렌더 장면에 복사할 광원 상태와 아이콘의 비소유 주소입니다.
     */
    FLightSceneProxy CreateLightProxy() const override;

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();
};
