/**
 * @file RegisterCoreUObject.cpp
 * @brief Core 및 CoreUObject 모듈의 리플렉션 등록 진입점입니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/Reflection/RegisterCoreUObject.h"
#include "CoreUObjectReflection.h"

void ReflectionModules::RegisterCoreUObject() {
    RegisterCoreUObjectTypes();
}
