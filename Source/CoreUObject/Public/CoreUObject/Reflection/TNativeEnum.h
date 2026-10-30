/**
 * @file TNativeEnum.h
 * @brief ADL을 통해 네이티브 열거형 메타데이터를 찾기 위한 타입 태그입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"

namespace CoreUObject {

    class UEnum;

    /**
     * @brief ADL을 통해 네이티브 열거형 메타데이터를 찾기 위한 타입 태그입니다.
     * @ingroup CoreUObject
     * @tparam T 네이티브 타입의 템플릿 인수입니다.
     */
    template <typename T>
    struct TNativeEnum {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(TNativeEnum);

    public:
        /**
         * @brief 이 템플릿 특수화의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectTemplate(TNativeEnum);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();
    };


    /**
     * @brief 네이티브 열거형의 정적 조회와 ADL 조회 함수를 선언합니다.
     * @param Type 생성, 조회 또는 표시할 타입입니다.
     */
#define ReflectEnum(Type, ...) CoreUObject::UEnum* StaticEnum##Type(); CoreUObject::UEnum* GetNativeEnum(CoreUObject::TNativeEnum<Type>)

}

#include "TNativeEnumReflection.generated.h"
