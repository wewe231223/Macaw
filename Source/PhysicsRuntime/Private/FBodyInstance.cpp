#include "PhysicsRuntime/FBodyInstance.h"

namespace PhysicsRuntime {
    FBodyInstance::FBodyInstance(physx::PxPhysics& Physics, PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose) {
        switch (BodyType) {
        case PhysicsRuntime::EBodyType::Static:
            mActor = Physics.createRigidStatic(Pose);
            break;
        case PhysicsRuntime::EBodyType::Kinematic:
        case PhysicsRuntime::EBodyType::Dynamic: {
            physx::PxRigidDynamic* Actor{Physics.createRigidDynamic(Pose)};

            if (Actor != nullptr && BodyType == PhysicsRuntime::EBodyType::Kinematic) {
                Actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
            }

            mActor = Actor;
            break;
        }
        }
    }

    FBodyInstance::~FBodyInstance() {
        if (mActor != nullptr) {
            mActor->release();
        }
    }

    physx::PxRigidActor* FBodyInstance::GetActor() const {
        return mActor;
    }
}
