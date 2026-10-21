#pragma once

#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/NativeReflection.h"
#include "CoreUObject/UObject.h"

class FObjectRegistry;

template <typename T, typename... TArguments>
std::unique_ptr<T> NewObject(FObjectRegistry& Registry, TArguments&&... Arguments);

template <typename T>
std::unique_ptr<UObject> CreateNativeInstance();

template <typename T>
UClass* DefineNativeClass(FString Name, UClass* (*Parent)(), EClassFlags Flags = EClassFlags::None);

#include "CoreUObject/Reflection/NewObjectTemplates.h"
