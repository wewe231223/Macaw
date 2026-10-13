#pragma once

#include "Core/Base/FName.h"
#include "CoreUObject/Reflection/EClassFlags.h"
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <utility>

class UObject;
class UClass;
class UScriptStruct;
class UEnum;
struct FNativeType;

class FReflectionRegistry {
public:
    using FFactory = std::unique_ptr<UObject> (*)();
    using FClassBuilder = void (*)(UClass&);
    using FStructBuilder = void (*)(UScriptStruct&);
    using FEnumBuilder = void (*)(UEnum&);

public:
    static UClass* DefineClass(FName Name, UClass* (*Parent)(), std::size_t Size, std::size_t Alignment, EClassFlags Flags, FFactory Factory, FClassBuilder Builder = nullptr);
    static UScriptStruct* DefineStruct(FName Name, const FNativeType* ValueType, FStructBuilder Builder = nullptr);
    static UEnum* DefineEnum(FName Name, const FNativeType* ValueType, FEnumBuilder Builder);
    static void AddEnumValues(UEnum& Information, std::initializer_list<std::pair<FName, Int64>> Values);
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
