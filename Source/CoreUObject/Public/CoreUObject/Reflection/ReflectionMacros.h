/**
 * @file ReflectionMacros.h
 * @brief 코드 생성에 사용하는 리플렉션 선언 또는 구현 매크로입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/NativeReflection.h"
#include "CoreUObject/Reflection/NewObject.h"

/**
 * @brief UObject 파생 타입의 클래스 메타데이터 조회 함수를 정의합니다.
 * @param Type 생성, 조회 또는 표시할 타입입니다.
 */
#define ImplementReflection(Type, ...) CoreUObject::UClass* Type::StaticClass() { \
    static CoreUObject::UClass* Class{CoreUObject::DefineNativeClass<Type>(#Type, &Super::StaticClass, CoreUObject::EClassFlags::None __VA_OPT__(| __VA_ARGS__))}; \
    return Class; \
} \
CoreUObject::UClass* Type::GetClass() const { \
    return StaticClass(); \
}

/**
 * @brief 부모가 없는 UObject 타입의 클래스 메타데이터 조회 함수를 정의합니다.
 * @param Type 생성, 조회 또는 표시할 타입입니다.
 */
#define ImplementRootReflection(Type, ...) CoreUObject::UClass* Type::StaticClass() { \
    static CoreUObject::UClass* Class{CoreUObject::DefineNativeClass<Type>(#Type, nullptr, CoreUObject::EClassFlags::None __VA_OPT__(| __VA_ARGS__))}; \
    return Class; \
} \
CoreUObject::UClass* Type::GetClass() const { \
    return StaticClass(); \
}

/**
 * @brief 네이티브 구조체의 정적 메타데이터 조회 함수를 정의합니다.
 * @param Type 생성, 조회 또는 표시할 타입입니다.
 */
#define ImplementStructReflection(Type) CoreUObject::UScriptStruct* Type::StaticStruct() { \
    return CoreUObject::DefineNativeStruct<ReflectionOwner>(#Type); \
}

/**
 * @brief 네이티브 열거값에서 이름과 정수 저장 값을 만듭니다.
 * @param Type 생성, 조회 또는 표시할 타입입니다.
 * @param Value 입력 값 또는 아카이브 모드에 따라 읽고 쓸 값입니다.
 */
#define ReflectEnumValue(Type, Value) CoreUObject::FEnumValue{#Value, static_cast<Core::Int32>(Type::Value)}

/**
 * @brief 열거형 등록 빌더와 ADL 메타데이터 조회 함수를 정의합니다.
 * @param Type 생성, 조회 또는 표시할 타입입니다.
 */
#define ImplementEnumReflection(Type, ...) void Build##Type##Reflection(CoreUObject::UEnum* Information) { \
    CoreUObject::FReflectionRegistry::AddEnumValues(Information, {__VA_ARGS__}); \
} \
CoreUObject::UEnum* StaticEnum##Type() { \
    static CoreUObject::UEnum* Enum{CoreUObject::FReflectionRegistry::DefineEnum(#Type, CoreUObject::GetNativeType<Type>(), &Build##Type##Reflection)}; \
    return Enum; \
} \
CoreUObject::UEnum* GetNativeEnum(CoreUObject::TNativeEnum<Type>) { \
    return StaticEnum##Type(); \
}
