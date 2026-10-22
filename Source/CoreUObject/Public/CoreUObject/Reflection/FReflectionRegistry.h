#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/UScriptStruct.h"
#include "CoreUObject/Reflection/UEnum.h"
#include "CoreUObject/Reflection/FNativeType.h"

class FReflectionRegistry {
public:
    ReflectTypes(FReflectionRegistry);

public:
    ReflectStruct(FReflectionRegistry);

    using FFactory = UClass::FFactory;
    using FClassBuilder = void (*)(UClass&);
    using FStructBuilder = void (*)(UScriptStruct&);
    using FEnumBuilder = void (*)(UEnum&);

public:
    static UClass* DefineClass(FString Name, UClass* (*Parent)(), const FNativeType* NativeType, EClassFlags Flags, FFactory Factory, FClassBuilder Builder);
    static UScriptStruct* DefineStruct(FString Name, const FNativeType* NativeType, FStructBuilder Builder, const UScriptStruct* Parent = nullptr);
    static UEnum* DefineEnum(FString Name, const FNativeType* NativeType, FEnumBuilder Builder);

    static void AddEnumValues(UEnum& Information, std::initializer_list<FEnumValue> Values);

    static UClass* FindClass(const FString& Name);
    static UScriptStruct* FindStruct(const FString& Name);
    static UEnum* FindEnum(const FString& Name);

private:
    ReflectMembers();
};
