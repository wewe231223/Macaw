/**
 * @file ACameraActor.h
 * @brief 카메라 컴포넌트를 갖고 이동·회전 입력을 처리하는 액터에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/AActor.h"

class UCameraComponent;

/**
 * @brief 카메라 컴포넌트를 갖고 이동·회전 입력을 처리하는 액터를 담당합니다.
 */
class ACameraActor : public AActor {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(ACameraActor, AActor);

public:
    /**
     * @brief 이 타입의 클래스 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(ACameraActor);
    bool InitializeDefaultComponents() override;
    /**
     * @brief 이 액터에 소속된 카메라 컴포넌트를 조회합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
     */
    UCameraComponent* GetCameraComponent() const;
    /**
     * @brief 뷰포트 입력을 받아 프레임 시간에 따라 액터의 동작을 갱신합니다.
     * @param[in] Input 현재 프레임의 키보드·마우스 입력 상태입니다.
     * @param[in] DeltaTime 이전 갱신 이후의 경과 시간이며 초 단위입니다.
     */
    void Input(const FViewportInput& Input, float DeltaTime) override;

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();
};
