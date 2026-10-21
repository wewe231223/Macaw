#pragma once

#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/FReflectionBuilder.h"
#include "CoreUObject/Reflection/TMemberPointerTraits.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

#include <type_traits>
#include <functional>

template <typename T>
const FNativeType* GetNativeType();

template <typename T>
EPropertyType GetNativePropertyType();

template <typename T>
const UStruct* GetNativeStruct();

template <typename T, typename TMetadata>
void BuildNativeReflection(TMetadata& Information);

template <auto Member, auto Getter = nullptr>
decltype(auto) ReadMemberValue(const typename TMemberPointerTraits<decltype(Member)>::FOwner& Object);

template <auto Member, auto Getter = nullptr>
FPropertyValue ReadMemberProperty(const void* Container);

template <auto Member, auto Getter = nullptr, auto Setter = nullptr>
bool WriteMemberProperty(void* Container, const FPropertyValue& Value);

template <auto Member, auto Getter = nullptr, auto Setter = nullptr>
std::unique_ptr<FProperty> MakeProperty(FReflectionBuilder& Owner, FString Name, EPropertyFlags Flags = EPropertyFlags::None);

template <auto Member, auto Getter = nullptr>
std::unique_ptr<FProperty> MakeReadOnlyProperty(FReflectionBuilder& Owner, FString Name, EPropertyFlags Flags = EPropertyFlags::None);

void RegisterProperty(FReflectionBuilder& Owner, std::unique_ptr<FProperty> Property);

#include "CoreUObject/Reflection/NativeReflectionTemplates.h"
