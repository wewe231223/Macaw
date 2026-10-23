/**
 * @file FPropertyChangedEvent.h
 * @brief 에디터에서 변경한 프로퍼티와 연속 편집 여부를 전달합니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"

class FProperty;

/**
 * @brief 에디터에서 변경한 프로퍼티와 연속 편집 여부를 전달합니다.
 * @ingroup CoreUObject
 */
struct FPropertyChangedEvent {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FPropertyChangedEvent);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FPropertyChangedEvent);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 변경한 프로퍼티의 비소유 주소입니다.
     */
    const FProperty* mProperty{};
    ReflectProperty(mProperty, "Property", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 드래그 등 연속 편집 중인지 여부입니다.
     */
    bool mInteractive{};
    ReflectProperty(mInteractive, "Interactive", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
