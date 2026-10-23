/**
 * @file USceneComponent.h
 * @brief 검증된 위치, 회전과 스케일을 보관하는 공간 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/Component/UActorComponent.h"
#include "Core/Math/FTransform.h"

/**
 * @brief 검증된 위치, 회전과 스케일을 보관하는 공간 컴포넌트입니다.
 * @ingroup Engine
 */
class USceneComponent : public UActorComponent {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(USceneComponent, UActorComponent);

public:
    /**
     * @brief USceneComponent의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    USceneComponent();
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~USceneComponent() override;

public:
    /**
     * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(USceneComponent);
    /**
     * @fn UClass* USceneComponent::GetClass() const
     * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
     * @return 실제 클래스 메타데이터의 비소유 주소입니다.
     */

    /**
     * @brief 컴포넌트의 위치·회전·스케일을 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const FTransform& GetTransform() const;
    /**
     * @brief 유한하고 유효한 변환을 검증하여 공간 컴포넌트에 적용합니다.
     * @param[in] Transform 적용할 위치, 회전과 스케일입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    virtual bool SetTransform(const FTransform& Transform);

    /**
     * @brief 위치, 회전과 스케일이 적용된 월드 행렬을 반환합니다.
     * @return 위치, 회전과 스케일이 적용된 월드 행렬입니다.
     */
    FMatrix GetWorldMatrix() const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 컴포넌트의 위치·회전·스케일입니다.
     */
    FTransform mTransform{};
    ReflectAccessor(mTransform, GetTransform, SetTransform, "Transform", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
