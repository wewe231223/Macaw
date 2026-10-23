/**
 * @file FWorldContext.h
 * @brief 하나의 월드와 그 객체 레지스트리를 함께 소유하는 수명 경계입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/FObjectRegistry.h"

class UWorld;

/**
 * @brief 하나의 월드와 그 객체 레지스트리를 함께 소유하는 수명 경계입니다.
 * @ingroup Engine
 */
class FWorldContext {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FWorldContext);

public:
    /**
     * @brief FWorldContext의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    FWorldContext();
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FWorldContext();

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FWorldContext);

    /**
     * @brief 객체를 등록한 레지스트리를 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FObjectRegistry& GetRegistry();
    /**
     * @brief 객체를 등록한 레지스트리를 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const FObjectRegistry& GetRegistry() const;

    /**
     * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    UWorld* GetWorld() const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 객체를 등록한 레지스트리입니다.
     */
    FObjectRegistry mRegistry{};
    ReflectProperty(mRegistry, "Registry", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 컨텍스트가 소유한 월드입니다.
     */
    std::unique_ptr<UWorld> mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
