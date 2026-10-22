#pragma once

#include "Core/ReflectionMacros.h"

template <typename T>
struct TNativeEnum {
public:
    ReflectTypes(TNativeEnum);

public:
    ReflectTemplate(TNativeEnum);

private:
    ReflectMembers();
};

class UEnum;

#define DeclareReflectEnum(Type) UEnum* StaticEnum##Type(); UEnum* GetNativeEnum(TNativeEnum<Type>)

template <typename T>
UScriptStruct* TNativeEnum<T>::StaticStruct() {
    return DefineTemplateStruct<ReflectionOwner>(__FUNCSIG__);
}
