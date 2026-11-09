/**
 * @file RegisterCoreUObject.cpp
 * @brief Core 및 CoreUObject 모듈의 리플렉션 등록 진입점입니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/Reflection/RegisterCoreUObject.h"
#include "CoreUObjectReflection.h"

namespace CoreUObject {

    /**
     * @brief Core와 CoreUObject의 생성된 리플렉션 등록을 실행합니다.
     */
    void CoreUObject::ReflectionModules::RegisterCoreUObject() {
        CoreUObject::ReflectionModules::RegisterCoreUObjectTypes();
    }

}
