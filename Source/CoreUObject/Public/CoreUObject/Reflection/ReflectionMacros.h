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
} \
void Type::RegisterReflectionMembers(FReflectionBuilder& Information)

#define ImplementRootReflection(Type, ...) UClass* Type::StaticClass() { \
    static UClass* Class{DefineNativeClass<Type>(#Type, nullptr, EClassFlags::None __VA_OPT__(| __VA_ARGS__))}; \
    return Class; \
} \
UClass* Type::GetClass() const { \
    return StaticClass(); \
} \
void Type::RegisterReflectionMembers(FReflectionBuilder& Information)

#define ImplementStructReflection(Type) UScriptStruct* Type::StaticStruct() { \
    static UScriptStruct* Struct{FReflectionRegistry::DefineStruct(#Type, GetNativeType<Type>(), &BuildNativeReflection<Type, UScriptStruct>)}; \
    return Struct; \
} \
void Type::RegisterReflectionMembers(FReflectionBuilder& Information)

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

#define ReflectProperty(Member, Name, Flags) RegisterProperty(Information, MakeProperty<&ReflectionOwner::Member>(Information, Name, Flags))
#define ReflectAccessor(Member, Getter, Setter, Name, Flags) RegisterProperty(Information, MakeProperty<&ReflectionOwner::Member, &ReflectionOwner::Getter, &ReflectionOwner::Setter>(Information, Name, Flags))
#define ReflectSetter(Member, Setter, Name, Flags) RegisterProperty(Information, MakeProperty<&ReflectionOwner::Member, nullptr, &ReflectionOwner::Setter>(Information, Name, Flags))
#define ReflectReadOnlyProperty(Member, Name, Flags) RegisterProperty(Information, MakeReadOnlyProperty<&ReflectionOwner::Member>(Information, Name, Flags))
#define ReflectReadOnlyAccessor(Member, Getter, Name, Flags) RegisterProperty(Information, MakeReadOnlyProperty<&ReflectionOwner::Member, &ReflectionOwner::Getter>(Information, Name, Flags))
