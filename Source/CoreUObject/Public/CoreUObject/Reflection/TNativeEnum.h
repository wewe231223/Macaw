/**
 * @file TNativeEnum.h
 * @brief ADL을 통해 네이티브 열거형 메타데이터를 찾기 위한 타입 태그입니다.
 * @ingroup CoreUObject
 */

#pragma once

namespace CoreUObject {

    class UEnum;

    /**
     * @brief ADL을 통해 네이티브 열거형 메타데이터를 찾기 위한 타입 태그입니다.
     * @ingroup CoreUObject
     * @tparam T 네이티브 타입의 템플릿 인수입니다.
     */
    template <typename T>
    struct TNativeEnum {
    };


    /**
     * @brief 네이티브 열거형의 정적 조회와 ADL 조회 함수를 선언합니다.
     * @param Type 리플렉션할 열거형 타입입니다.
     */
#define ReflectEnum(Type, ...) CoreUObject::UEnum* StaticEnum##Type(); CoreUObject::UEnum* GetNativeEnum(CoreUObject::TNativeEnum<Type>)

}
