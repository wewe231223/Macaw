#include "pch.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/UFunction.h"
#include "CoreUObject/UObject.h"

void ReflectionModules::RegisterCoreUObject() {
    UField::StaticClass();
    UStruct::StaticClass();
    UClass::StaticClass();
    UScriptStruct::StaticClass();
    UEnum::StaticClass();
    UFunction::StaticClass();
    UObject::StaticClass();
}
