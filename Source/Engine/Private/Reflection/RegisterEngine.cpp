/**
 * @file RegisterEngine.cpp
 * @brief Engine 모듈의 리플렉션 등록 진입점입니다.
 * @ingroup Engine
 */

#include "Engine/Reflection/RegisterEngine.h"
#include "CoreUObject/Reflection/RegisterCoreUObject.h"
#include "EngineReflection.h"

void ReflectionModules::RegisterEngine() {
    RegisterCoreUObject();
    RegisterEngineTypes();
}
