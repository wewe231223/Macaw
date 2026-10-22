#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/FReflectionBuilder.h"
#include "CoreUObject/Reflection/TMemberPointerTraits.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

#include <type_traits>
#include <functional>
#include <limits>

template <typename T>
const FNativeType* GetNativeType();

template <typename T>
constexpr EPropertyType GetNativePropertyType();

template <typename T>
const UStruct* GetNativeStruct();

template <typename T>
UScriptStruct* DefineNativeStruct(const char* Name);

template <typename T, typename TMetadata>
void BuildNativeReflection(TMetadata& Information);

template <auto Member, auto Getter = nullptr>
decltype(auto) ReadMemberValue(const typename TMemberPointerTraits<decltype(Member)>::FOwner& Object);

template <auto Member, auto Getter = nullptr>
FPropertyValue ReadMemberProperty(const void* Container);

template <auto Member, auto Getter = nullptr, auto Setter = nullptr>
bool WriteMemberProperty(void* Container, const FPropertyValue& Value);

template <auto Member, auto Getter = nullptr, auto Setter = nullptr>
void RegisterProperty(FReflectionBuilder& Information, FString Name, EPropertyFlags Flags = EPropertyFlags::None);

template <auto Member, auto Getter = nullptr>
void RegisterReadOnlyProperty(FReflectionBuilder& Information, FString Name, EPropertyFlags Flags = EPropertyFlags::None);

#include "CoreUObject/Reflection/NativeReflectionTemplates.h"
