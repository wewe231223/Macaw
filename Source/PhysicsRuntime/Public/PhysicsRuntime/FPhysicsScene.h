#pragma once

#include "PhysicsRuntime/EBodyType.h"
#include "PhysicsRuntime/FBodyInstance.h"
#include "PhysicsRuntime/UPhysicalMaterial.h"
#include <PxPhysicsAPI.h>
#include <memory>

namespace PhysicsRuntime {
    class FPhysicsRuntime;

    class FPhysicsScene {
    public:
        FPhysicsScene(PhysicsRuntime::FPhysicsRuntime& Runtime, physx::PxPhysics& Physics, const physx::PxSceneDesc& Description);
        ~FPhysicsScene();
        FPhysicsScene(const FPhysicsScene&) = delete;
        FPhysicsScene& operator=(const FPhysicsScene&) = delete;

    public:
        bool IsValid() const;

        std::unique_ptr<PhysicsRuntime::FBodyInstance> CreateBody(PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose, const physx::PxGeometry& Geometry, const physx::PxTransform& ShapePose = physx::PxTransform{physx::PxIdentity}, const PhysicsRuntime::UPhysicalMaterial& Material = PhysicsRuntime::UPhysicalMaterial::GetDefault());
        bool SetBodyMaterial(const PhysicsRuntime::FBodyInstance& Body, const PhysicsRuntime::UPhysicalMaterial& Material);

        bool Step(float DeltaSeconds);

    private:
        PhysicsRuntime::FPhysicsRuntime& mRuntime;
        physx::PxScene* mScene{};
    };
}
