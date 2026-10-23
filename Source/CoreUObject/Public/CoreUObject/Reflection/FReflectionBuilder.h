/**
 * @file FReflectionBuilder.h
 * @brief 생성된 등록 코드가 소유 타입에 프로퍼티를 추가하는 진입점입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

class UStruct;
class FProperty;

/**
 * @brief 생성된 등록 코드가 소유 타입에 프로퍼티를 추가하는 진입점입니다.
 * @ingroup CoreUObject
 */
class FReflectionBuilder {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FReflectionBuilder);

public:
    /**
     * @brief FReflectionBuilder의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in,out] Owner 컴포넌트 또는 메타데이터의 소유자입니다.
     */
    explicit FReflectionBuilder(UStruct* Owner);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FReflectionBuilder);

    /**
     * @brief 소유 객체 또는 메타데이터를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    UStruct* GetOwner() const;
    /**
     * @brief 타입 메타데이터에 프로퍼티를 추가합니다.
     * @param[in] Property 읽기, 쓰기 또는 알림을 적용할 프로퍼티 메타데이터입니다.
     */
    void AddProperty(std::unique_ptr<FProperty> Property);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 소유 객체 또는 메타데이터입니다.
     */
    UStruct* mOwner{};
    ReflectProperty(mOwner, "Owner", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
