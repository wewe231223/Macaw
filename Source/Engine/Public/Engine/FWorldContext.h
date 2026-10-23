/**
 * @file FWorldContext.h
 * @brief 하나의 월드와 엔진 식별자를 연결하는 수명 경계입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/EWorldType.h"
#include "Engine/FWorldHandle.h"

class UWorld;
class UEngine;

/**
 * @brief 하나의 월드와 엔진 식별자를 연결하는 수명 경계입니다.
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
    /**
     * @brief 지정한 용도의 월드와 영속 레벨을 생성합니다.
     * @param[in] Type 소유 월드의 편집 또는 실행 용도입니다.
     */
    explicit FWorldContext(EWorldType Type = EWorldType::Editor);
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
     * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    UWorld* GetWorld() const;
    /**
     * @brief 엔진에 등록된 컨텍스트의 월드 핸들을 반환합니다.
     * @return 엔진 등록 전에는 무효인 비소유 핸들입니다.
     */
    FWorldHandle GetHandle() const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();
    friend class UEngine;
    /**
     * @brief 엔진이 발급한 식별자를 컨텍스트와 월드에 함께 반영합니다.
     * @param[in] Handle 엔진 슬롯과 발급 세대를 결합한 핸들입니다.
     */
    void SetHandle(FWorldHandle Handle);

private:
    /**
     * @brief 컨텍스트가 소유한 월드입니다.
     */
    std::unique_ptr<UWorld> mWorld{};
    ReflectProperty(mWorld, "World", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 컨텍스트와 소유 월드가 함께 사용하는 엔진 발급 식별자입니다.
     */
    FWorldHandle mHandle{};
    ReflectProperty(mHandle, "Handle", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
