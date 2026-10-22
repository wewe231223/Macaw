#include "Engine/Reflection/RegisterEngine.h"
#include "CoreUObject/Reflection/RegisterCoreUObject.h"
#include "EngineReflection.h"

void ReflectionModules::RegisterEngine() {
    RegisterCoreUObject();
    RegisterEngineTypes();
}
