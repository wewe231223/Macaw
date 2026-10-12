#pragma once

#include "CoreUObject/Reflection/UClass.h"
#include <functional>

class FReflectionRegistry {
public:
    using FClassBuilder = void (*)(UClass&);
    using FStructBuilder = void (*)(UScriptStruct&);
    using FEnumBuilder = void (*)(UEnum&);

public:
    static UClass* DefineClass(FName Name, UClass* (*Parent)(), std::size_t Size, std::size_t Alignment, EClassFlags Flags, UClass::FFactory Factory, FClassBuilder Builder = nullptr);
    static UScriptStruct* DefineStruct(FName Name, const FNativeType* ValueType, FStructBuilder Builder = nullptr);
    static UEnum* DefineEnum(FName Name, const FNativeType* ValueType, FEnumBuilder Builder);
    static UClass* FindClass(FName Name);
    static UScriptStruct* FindStruct(FName Name);
    static UEnum* FindEnum(FName Name);
    static UEnum* FindEnum(const FNativeType* ValueType);
    static TArray<const UClass*> GetClasses();
    static bool IsBootstrapping();
};

namespace ReflectionModules {
    void RegisterCoreUObject();
    void RegisterRenderCore();
    void RegisterAsset();
    void RegisterWorld();
    void RegisterEngine();
    void RegisterEditor();
}
