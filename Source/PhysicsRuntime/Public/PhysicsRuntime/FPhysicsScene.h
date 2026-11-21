#pragma once

#include "PhysicsRuntime/EBodyType.h"
#include "PhysicsRuntime/FBodyInstance.h"
#include <PxPhysicsAPI.h>
#include <memory>

namespace PhysicsRuntime {
    class FPhysicsScene {
    public:
        FPhysicsScene(physx::PxPhysics& Physics, const physx::PxSceneDesc& Description);
        ~FPhysicsScene();
        FPhysicsScene(const FPhysicsScene&) = delete;
        FPhysicsScene& operator=(const FPhysicsScene&) = delete;

    public:
        bool IsValid() const;

        std::unique_ptr<PhysicsRuntime::FBodyInstance> CreateBody(PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose, const physx::PxGeometry& Geometry, const physx::PxTransform& ShapePose = physx::PxTransform{physx::PxIdentity});

    private:
        physx::PxScene* mScene{};
        physx::PxMaterial* mDefaultMaterial{};
    };
}
