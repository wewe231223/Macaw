#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "ECollisionEnabled.generated.h"

namespace PhysicsRuntime {
    UENUM()
    enum class ECollisionEnabled : Uint8 {
        NoCollision,
        QueryOnly,
        PhysicsOnly,
        QueryAndPhysics
    };

    bool HasQueryCollision(PhysicsRuntime::ECollisionEnabled CollisionEnabled);
    bool HasPhysicsCollision(PhysicsRuntime::ECollisionEnabled CollisionEnabled);
}
