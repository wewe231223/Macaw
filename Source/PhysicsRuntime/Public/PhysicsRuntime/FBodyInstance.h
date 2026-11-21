#pragma once

#include "PhysicsRuntime/EBodyType.h"
#include <PxPhysicsAPI.h>

namespace PhysicsRuntime {
    class FBodyInstance {
    public:
        FBodyInstance(physx::PxPhysics& Physics, PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose);
        ~FBodyInstance();
        FBodyInstance(const FBodyInstance&) = delete;
        FBodyInstance& operator=(const FBodyInstance&) = delete;

    public:
        physx::PxRigidActor* GetActor() const;

    private:
        physx::PxRigidActor* mActor{};
    };
}
