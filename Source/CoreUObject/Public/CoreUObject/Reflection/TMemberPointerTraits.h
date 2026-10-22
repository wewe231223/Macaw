#pragma once

#include "Core/ReflectionMacros.h"

template <typename T>
struct TMemberPointerTraits;

template <typename TValue, typename TOwner>
struct TMemberPointerTraits<TValue TOwner::*> {
public:
    ReflectTypes(TMemberPointerTraits);

public:
    using FOwner = TOwner;
    using FValue = TValue;

public:
    ReflectTemplate(TMemberPointerTraits);

private:
    ReflectMembers();
};

template <typename TValue, typename TOwner>
UScriptStruct* TMemberPointerTraits<TValue TOwner::*>::StaticStruct() {
    return DefineTemplateStruct<ReflectionOwner>(__FUNCSIG__);
}
