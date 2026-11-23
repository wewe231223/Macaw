#include "PhysicsRuntime/ECollisionEnabled.h"

namespace PhysicsRuntime {
    bool HasQueryCollision(PhysicsRuntime::ECollisionEnabled CollisionEnabled) {
        return CollisionEnabled == ECollisionEnabled::QueryOnly || CollisionEnabled == ECollisionEnabled::QueryAndPhysics;
    }

    bool HasPhysicsCollision(PhysicsRuntime::ECollisionEnabled CollisionEnabled) {
        return CollisionEnabled == ECollisionEnabled::PhysicsOnly || CollisionEnabled == ECollisionEnabled::QueryAndPhysics;
    }
}
