#pragma once

template <typename T>
struct TNativeEnum {
};

class UEnum;

#define DeclareReflectEnum(Type) UEnum* StaticEnum##Type(); UEnum* GetNativeEnum(TNativeEnum<Type>)
