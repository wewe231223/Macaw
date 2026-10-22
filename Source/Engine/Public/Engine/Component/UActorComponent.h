/**
 * @file UActorComponent.h
 * @brief 소유 액터와 월드 등록 수명을 갖는 컴포넌트의 기반 클래스입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/UObject.h"

class AActor;
class UWorld;

/**
 * @brief 소유 액터와 월드 등록 수명을 갖는 컴포넌트의 기반 클래스입니다.
 * @ingroup Engine
 */
class UActorComponent : public UObject {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UActorComponent, UObject);

public:
    /**
     * @brief UActorComponent의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    UActorComponent();
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~UActorComponent() override;

public:
    /**
     * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(UActorComponent, EClassFlags::NoFactory);
    /**
     * @fn UClass* UActorComponent::GetClass() const
     * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
     * @return 실제 클래스 메타데이터의 비소유 주소입니다.
     */

    /**
     * @brief 소유 객체 또는 메타데이터를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    AActor* GetOwner() const;
    /**
     * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    UWorld* GetWorld() const;
    /**
     * @brief 컴포넌트의 소유 액터를 지정합니다.
     * @param[in] Owner 컴포넌트 또는 메타데이터의 소유자입니다.
     */
    void SetOwner(AActor* Owner);

    /**
     * @brief 컴포넌트를 월드에 연결하고 필요한 렌더 상태를 등록합니다.
     * @param[in,out] World 연결할 월드 또는 적용할 월드 행렬입니다.
     */
    virtual void RegisterComponent(UWorld& World);
    /**
     * @brief 등록된 렌더 상태와 월드 연결을 해제합니다.
     */
    virtual void UnregisterComponent();
    /**
     * @brief 컴포넌트가 월드에 등록되어 있는지 검사합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsRegistered() const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 소유 객체 또는 메타데이터입니다.
     */
    AActor* mOwner{};
    ReflectProperty(mOwner, "Owner", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 월드 연결 또는 월드 변환 행렬입니다.
     */
    UWorld* mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
