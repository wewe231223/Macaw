/**
 * @file RegisterEngine.cpp
 * @brief Engine 모듈의 리플렉션 등록 진입점입니다.
 * @ingroup Engine
 */

#include "Engine/Reflection/RegisterEngine.h"
#include "CoreUObject/Reflection/RegisterCoreUObject.h"
#include "EngineReflection.h"

namespace Engine {

    /**
     * @brief Engine 모듈의 생성된 리플렉션 등록을 실행합니다.
     */
    void Engine::ReflectionModules::RegisterEngine() {
        CoreUObject::ReflectionModules::RegisterCoreUObject();
        Engine::ReflectionModules::RegisterEngineTypes();
    }

}
