/**
 * @file APointLightActor.h
 * @brief 점광원 컴포넌트를 기본으로 생성하는 액터입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/AActor.h"

class UPointLightComponent;

/**
 * @brief 점광원 컴포넌트를 기본으로 생성하는 액터입니다.
 * @ingroup Engine
 */
class APointLightActor final : public AActor {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(APointLightActor, AActor);

public:
    /**
     * @brief APointLightActor의 기본 상태를 초기화합니다.
     */
    APointLightActor();

    /**
     * @brief APointLightActor 객체의 수명을 끝내고 소유 자원을 정리합니다.
     */
    ~APointLightActor() override;

public:
    /**
     * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 클래스 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(APointLightActor);

    /**
     * @brief 액터 종류에 맞는 기본 광원 컴포넌트를 생성하고 루트에 연결합니다.
     * @return 새 컴포넌트를 연결하면 true이며 이미 컴포넌트가 있거나 연결에 실패하면 false입니다.
     */
    bool InitializeDefaultComponents() override;

    /**
     * @brief 액터의 기본 광원 컴포넌트를 조회합니다.
     * @return 액터가 소유한 광원 컴포넌트의 비소유 주소이며 없으면 nullptr입니다.
     */
    UPointLightComponent* GetLightComponent() const;

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();
};
