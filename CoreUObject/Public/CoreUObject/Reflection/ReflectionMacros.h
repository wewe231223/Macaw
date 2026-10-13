#pragma once

#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/Reflection/NativeReflection.h"

class UScriptStruct;
class UEnum;

#define ReflectTypes(Type, ...) using ReflectionOwner = Type __VA_OPT__(; using Super = __VA_ARGS__); enum : std::size_t { \
    ReflectionBegin = __LINE__ \
}
#define ReflectClass(Type, ...) static UClass* StaticClass() { \
    return DefineNativeClass<Type>(FName{#Type}, &Super::StaticClass, EClassFlags::None __VA_OPT__(| __VA_ARGS__), &BuildNativeReflection<Type, UClass>); \
}
#define ReflectRootClass(Type, ...) static UClass* StaticClass() { \
    return DefineNativeClass<Type>(FName{#Type}, nullptr, EClassFlags::None __VA_OPT__(| __VA_ARGS__), &BuildNativeReflection<Type, UClass>); \
}
#define ReflectStruct(Type) static UScriptStruct* StaticStruct() { \
    return FReflectionRegistry::DefineStruct(FName{#Type}, GetNativeType<Type>(), &BuildNativeReflection<Type, UScriptStruct>); \
}
#define ReflectEnum(Type, ...) inline void Build##Type##Reflection(UEnum& Information) { \
    FReflectionRegistry::AddEnumValues(Information, {__VA_ARGS__}); \
} \
inline UEnum* StaticEnum##Type() { \
    return FReflectionRegistry::DefineEnum(FName{#Type}, GetNativeType<Type>(), &Build##Type##Reflection); \
}
#define ReflectEnumValue(Type, Value) std::pair<FName, Int64>{FName{#Value}, static_cast<Int64>(Type::Value)}
#define ReflectProperty(Member, Name, Flags, ...) friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, __LINE__>, FReflectionBuilder& Information) { \
    RegisterProperty(Information, MakeProperty<&ReflectionOwner::Member>(Information, FName{Name}, Flags) __VA_OPT__(, __VA_ARGS__)); \
}
#define ReflectAccessor(Member, Getter, Setter, Name, Flags, ...) friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, __LINE__>, FReflectionBuilder& Information) { \
    RegisterProperty(Information, MakeProperty<&ReflectionOwner::Member, &ReflectionOwner::Getter, &ReflectionOwner::Setter>(Information, FName{Name}, Flags) __VA_OPT__(, __VA_ARGS__)); \
}
#define ReflectSetter(Member, Setter, Name, Flags, ...) friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, __LINE__>, FReflectionBuilder& Information) { \
    RegisterProperty(Information, MakeProperty<&ReflectionOwner::Member, nullptr, &ReflectionOwner::Setter>(Information, FName{Name}, Flags) __VA_OPT__(, __VA_ARGS__)); \
}
#define ReflectReadOnlyProperty(Member, Name, Flags, ...) friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, __LINE__>, FReflectionBuilder& Information) { \
    RegisterProperty(Information, MakeReadOnlyProperty<&ReflectionOwner::Member>(Information, FName{Name}, Flags) __VA_OPT__(, __VA_ARGS__)); \
}
#define ReflectReadOnlyAccessor(Member, Getter, Name, Flags, ...) friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, __LINE__>, FReflectionBuilder& Information) { \
    RegisterProperty(Information, MakeReadOnlyProperty<&ReflectionOwner::Member, &ReflectionOwner::Getter>(Information, FName{Name}, Flags) __VA_OPT__(, __VA_ARGS__)); \
}
#define ReflectSerializer(Member) &SerializeMemberProperty<&ReflectionOwner::Member>
#define ReflectFunction(Method, ...) friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, __LINE__>, FReflectionBuilder& Information) { \
    RegisterFunction<&ReflectionOwner::Method>(Information, FName{#Method} __VA_OPT__(, {__VA_ARGS__})); \
}
#define ReflectEditor(Category, DisplayName) [](FProperty& Property) { \
    Property.SetEditorMetadata(FString{Category}, FString{DisplayName}, Property.GetMinimum(), Property.GetMaximum(), Property.GetSpeed()); \
}
#define ReflectRange(Minimum, Maximum) [](FProperty& Property) { \
    Property.SetEditorMetadata(Property.GetCategory(), Property.GetDisplayName(), Minimum, Maximum, Property.GetSpeed()); \
}
#define ReflectSpeed(Speed) [](FProperty& Property) { \
    Property.SetEditorMetadata(Property.GetCategory(), Property.GetDisplayName(), Property.GetMinimum(), Property.GetMaximum(), Speed); \
}
#define ReflectBound(Method) &ReadPropertyMetadata<&ReflectionOwner::Method, double>
#define ReflectBounds(Minimum, Maximum) [](FProperty& Property) { \
    Property.SetDynamicBounds(Minimum, Maximum); \
}
#define ReflectCondition(Method) [](FProperty& Property) { \
    Property.SetEditCondition(&ReadPropertyMetadata<&ReflectionOwner::Method, bool>); \
}
#define ReflectOptions(Method) [](FProperty& Property) { \
    Property.SetOptionsGetter(&ReadPropertyMetadata<&ReflectionOwner::Method, TArray<FName>>); \
}
#define ReflectAsset(Type) [](FProperty& Property) { \
    Property.SetAssetClass(Type::StaticClass()); \
}
#define ReflectFilter(Method) [](FProperty& Property) { \
    Property.SetObjectFilter(&FilterPropertyObject<&ReflectionOwner::Method>); \
}
#define ReflectSerializationName(Name) [](FProperty& Property) { \
    Property.SetSerializationName(FString{Name}); \
}
#define ReflectSerializationScope(Scope) [](FProperty& Property) { \
    Property.SetSerializationScope(FString{Scope}); \
}
#define ReflectMembers() friend void RegisterReflectionMember(TReflectionMember<ReflectionOwner, 0>, FReflectionBuilder& Information) { \
    RegisterReflectionMembers<ReflectionOwner, ReflectionBegin, __LINE__>(Information); \
}
