/**
 * @file FRegisteredType.h
 * @brief 타입 정의와 소유 중인 리플렉션 메타데이터를 묶은 등록 항목입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "FReflectionDefinition.h"

/**
 * @brief 타입 정의와 소유 중인 리플렉션 메타데이터를 묶은 등록 항목입니다.
 * @ingroup CoreUObject
 */
struct FRegisteredType {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FRegisteredType);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FRegisteredType);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 중복 등록 검사에 사용하는 타입 정의입니다.
     */
    FReflectionDefinition mDefinition{};
    ReflectProperty(mDefinition, "Definition", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 소유 중인 타입 메타데이터입니다.
     */
    std::unique_ptr<UStruct> mInformation{};
    ReflectProperty(mInformation, "Information", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
