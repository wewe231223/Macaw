#pragma once

#include "Core/ReflectionMacros.h"
#include "EReflectionType.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"

struct FReflectionDefinition {
public:
    ReflectTypes(FReflectionDefinition);

public:
    ReflectStruct(FReflectionDefinition);

    bool operator==(const FReflectionDefinition& Other) const;

private:
    ReflectMembers();

public:
    EReflectionType mKind{};
    ReflectProperty(mKind, "Kind", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    const FNativeType* mNativeType{};
    ReflectProperty(mNativeType, "NativeType", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    UClass* (*mParent)(){};
    ReflectProperty(mParent, "Parent", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    EClassFlags mFlags{};
    ReflectProperty(mFlags, "Flags", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FReflectionRegistry::FFactory mFactory{};
    ReflectProperty(mFactory, "Factory", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FReflectionRegistry::FClassBuilder mClassBuilder{};
    ReflectProperty(mClassBuilder, "ClassBuilder", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FReflectionRegistry::FStructBuilder mStructBuilder{};
    ReflectProperty(mStructBuilder, "StructBuilder", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FReflectionRegistry::FEnumBuilder mEnumBuilder{};
    ReflectProperty(mEnumBuilder, "EnumBuilder", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    const UScriptStruct* mStructParent{};
    ReflectProperty(mStructParent, "StructParent", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
