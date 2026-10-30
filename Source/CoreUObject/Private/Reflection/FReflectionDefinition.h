/**
 * @file FReflectionDefinition.h
 * @brief 중복 등록의 호환성을 검사하는 네이티브 타입 정의입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "EReflectionType.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"

namespace CoreUObject {

    /**
     * @brief 중복 등록의 호환성을 검사하는 네이티브 타입 정의입니다.
     * @ingroup CoreUObject
     */
    struct FReflectionDefinition {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FReflectionDefinition);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FReflectionDefinition);

        /**
         * @brief 저장된 성분 또는 식별 상태가 정확히 같은지 검사합니다.
         * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool operator==(const CoreUObject::FReflectionDefinition& Other) const;

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 등록 메타데이터 종류입니다.
         */
        CoreUObject::EReflectionType mKind{};
        ReflectProperty(mKind, "Kind", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 네이티브 타입 식별 메타데이터입니다.
         */
        const CoreUObject::FNativeType* mNativeType{};
        ReflectProperty(mNativeType, "NativeType", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 부모 타입 또는 부모 조회 콜백입니다.
         */
        CoreUObject::UClass* (*mParent)(){};
        ReflectProperty(mParent, "Parent", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 동작 정책 비트 플래그입니다.
         */
        CoreUObject::EClassFlags mFlags{};
        ReflectProperty(mFlags, "Flags", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 객체 생성 팩터리입니다.
         */
        CoreUObject::FReflectionRegistry::FFactory mFactory{};
        ReflectProperty(mFactory, "Factory", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 클래스 멤버 등록 콜백입니다.
         */
        CoreUObject::FReflectionRegistry::FClassBuilder mClassBuilder{};
        ReflectProperty(mClassBuilder, "ClassBuilder", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 구조체 멤버 등록 콜백입니다.
         */
        CoreUObject::FReflectionRegistry::FStructBuilder mStructBuilder{};
        ReflectProperty(mStructBuilder, "StructBuilder", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 열거값 등록 콜백입니다.
         */
        CoreUObject::FReflectionRegistry::FEnumBuilder mEnumBuilder{};
        ReflectProperty(mEnumBuilder, "EnumBuilder", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 부모 구조체 메타데이터입니다.
         */
        const CoreUObject::UScriptStruct* mStructParent{};
        ReflectProperty(mStructParent, "StructParent", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
