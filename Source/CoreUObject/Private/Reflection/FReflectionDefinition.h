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
         * @brief 종류, 네이티브 타입, 부모, 플래그, 팩터리와 빌더가 모두 같은지 검사합니다.
         * @param[in] Other 비교할 상대 값입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool operator==(const CoreUObject::FReflectionDefinition& Other) const;

    public:
        /**
         * @brief 등록 메타데이터 종류입니다.
         */
        CoreUObject::EReflectionType mKind{};

        /**
         * @brief 네이티브 타입 식별 메타데이터입니다.
         */
        const CoreUObject::FNativeType* mNativeType{};

        /**
         * @brief 부모 클래스 메타데이터를 반환하는 함수입니다.
         */
        CoreUObject::UClass* (*mParent)(){};

        /**
         * @brief 동작 정책 비트 플래그입니다.
         */
        CoreUObject::EClassFlags mFlags{};

        /**
         * @brief 객체 생성 팩터리입니다.
         */
        CoreUObject::FReflectionRegistry::FFactory mFactory{};

        /**
         * @brief 클래스 멤버 등록 콜백입니다.
         */
        CoreUObject::FReflectionRegistry::FClassBuilder mClassBuilder{};

        /**
         * @brief 구조체 멤버 등록 콜백입니다.
         */
        CoreUObject::FReflectionRegistry::FStructBuilder mStructBuilder{};

        /**
         * @brief 열거값 등록 콜백입니다.
         */
        CoreUObject::FReflectionRegistry::FEnumBuilder mEnumBuilder{};

        /**
         * @brief 부모 구조체 메타데이터입니다.
         */
        const CoreUObject::UScriptStruct* mStructParent{};
    };

}
