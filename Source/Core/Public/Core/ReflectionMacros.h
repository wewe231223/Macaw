#pragma once

#include <type_traits>

class UClass;
class UScriptStruct;
class FReflectionBuilder;

template <typename T, typename TMetadata>
void BuildNativeReflection(TMetadata& Information);

template <typename T>
UScriptStruct* DefineTemplateStruct(const char* Name);

#define ReflectTypes(Type, ...) using ReflectionOwner = Type __VA_OPT__(; using Super = __VA_ARGS__)
#define ReflectRootClass(Type) static_assert(std::is_same_v<ReflectionOwner, Type>); static UClass* StaticClass(); virtual UClass* GetClass() const
#define ReflectClass(Type) static_assert(std::is_same_v<ReflectionOwner, Type>); static UClass* StaticClass(); UClass* GetClass() const override
#define ReflectStruct(Type) static_assert(std::is_same_v<ReflectionOwner, Type>); static UScriptStruct* StaticStruct()
#define ReflectTemplate(Type) ReflectStruct(Type)
#define ReflectMembers() template <typename T, typename TMetadata> friend void ::BuildNativeReflection(TMetadata& Information); static void RegisterReflectionMembers(FReflectionBuilder& Information)

#define ReflectProperty(Member, Name, Flags)
#define ReflectAccessor(Member, Getter, Setter, Name, Flags)
#define ReflectSetter(Member, Setter, Name, Flags)
#define ReflectReadOnlyProperty(Member, Name, Flags)
#define ReflectReadOnlyAccessor(Member, Getter, Name, Flags)
