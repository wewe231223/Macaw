#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "EBodyType.generated.h"

namespace PhysicsRuntime {
    UENUM()
    enum class EBodyType : Uint8 {
        Static,
        Kinematic,
        Dynamic
    };
}
