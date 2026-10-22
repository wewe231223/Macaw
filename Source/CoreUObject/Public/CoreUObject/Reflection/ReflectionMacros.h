#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/NativeReflection.h"
#include "CoreUObject/Reflection/NewObject.h"

#define ImplementReflection(Type, ...) UClass* Type::StaticClass() { \
    static UClass* Class{DefineNativeClass<Type>(#Type, &Super::StaticClass, EClassFlags::None __VA_OPT__(| __VA_ARGS__))}; \
    return Class; \
} \
UClass* Type::GetClass() const { \
    return StaticClass(); \
}

#define ImplementRootReflection(Type, ...) UClass* Type::StaticClass() { \
    static UClass* Class{DefineNativeClass<Type>(#Type, nullptr, EClassFlags::None __VA_OPT__(| __VA_ARGS__))}; \
    return Class; \
} \
UClass* Type::GetClass() const { \
    return StaticClass(); \
}

#define ImplementStructReflection(Type) UScriptStruct* Type::StaticStruct() { \
    return DefineNativeStruct<ReflectionOwner>(#Type); \
}

#define ReflectEnumValue(Type, Value) FEnumValue{#Value, static_cast<Int32>(Type::Value)}
#define ReflectEnum(Type, ...) void Build##Type##Reflection(UEnum& Information) { \
    FReflectionRegistry::AddEnumValues(Information, {__VA_ARGS__}); \
} \
UEnum* StaticEnum##Type() { \
    static UEnum* Enum{FReflectionRegistry::DefineEnum(#Type, GetNativeType<Type>(), &Build##Type##Reflection)}; \
    return Enum; \
} \
UEnum* GetNativeEnum(TNativeEnum<Type>) { \
    return StaticEnum##Type(); \
}
