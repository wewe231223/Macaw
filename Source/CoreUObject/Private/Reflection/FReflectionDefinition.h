#pragma once

#include "EReflectionType.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"

struct FReflectionDefinition {
public:
    bool operator==(const FReflectionDefinition& Other) const;

public:
    EReflectionType mKind{};
    const FNativeType* mNativeType{};

    UClass* (*mParent)(){};
    EClassFlags mFlags{};
    FReflectionRegistry::FFactory mFactory{};

    FReflectionRegistry::FClassBuilder mClassBuilder{};
    FReflectionRegistry::FStructBuilder mStructBuilder{};
    FReflectionRegistry::FEnumBuilder mEnumBuilder{};
};
