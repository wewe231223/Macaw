/**
 * @file AActor.h
 * @brief 컴포넌트를 소유하고 루트 컴포넌트를 통해 공간 변환을 제공하는 월드 객체입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/UObject.h"

class UWorld;
class UActorComponent;
class USceneComponent;
struct FTransform;

/**
 * @brief 컴포넌트를 소유하고 루트 컴포넌트를 통해 공간 변환을 제공하는 월드 객체입니다.
 * @ingroup Engine
 */
class AActor : public UObject {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(AActor, UObject);

public:
    /**
     * @brief AActor의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    AActor();
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~AActor() override;

public:
    /**
     * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(AActor, EClassFlags::NoFactory);
    /**
     * @fn UClass* AActor::GetClass() const
     * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
     * @return 실제 클래스 메타데이터의 비소유 주소입니다.
     */

    /**
     * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    UWorld* GetWorld() const;
    /**
     * @brief 객체가 소속된 월드 연결을 지정합니다.
     * @param[in] World 연결할 월드 또는 적용할 월드 행렬입니다.
     */
    void SetWorld(UWorld* World);

    /**
     * @brief 컴포넌트의 소유권을 액터로 옮기고 필요하면 월드에 등록합니다.
     * @param[in] Component 액터가 소유권을 넘겨받을 컴포넌트입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool AddComponent(std::unique_ptr<UActorComponent> Component);
    /**
     * @brief 액터가 소유한 컴포넌트 목록을 반환합니다.
     * @return 생성한 객체의 단독 소유권이며, 생성 또는 검증 실패 시 nullptr입니다.
     */
    const TArray<std::unique_ptr<UActorComponent>>& GetComponents() const;

    /**
     * @brief 액터 변환을 제공하는 루트 컴포넌트의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    USceneComponent* GetRootComponent() const;
    /**
     * @brief 루트 공간 컴포넌트에 액터의 변환을 적용합니다.
     * @param[in] Transform 적용할 위치, 회전과 스케일입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool SetActorTransform(const FTransform& Transform);

    /**
     * @brief 액터의 기본 컴포넌트를 생성하고 루트 컴포넌트를 정합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    virtual bool InitializeDefaultComponents();

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 월드 연결 또는 월드 변환 행렬입니다.
     */
    UWorld* mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 액터가 소유한 컴포넌트 목록입니다.
     */
    TArray<std::unique_ptr<UActorComponent>> mComponents{};
    ReflectProperty(mComponents, "Components", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 액터 변환을 제공하는 루트 컴포넌트의 비소유 주소입니다.
     */
    USceneComponent* mRootComponent{};
    ReflectProperty(mRootComponent, "RootComponent", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
