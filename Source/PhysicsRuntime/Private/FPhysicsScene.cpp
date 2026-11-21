#include "PhysicsRuntime/FPhysicsScene.h"
#include "Core/Literals/UnitLiterals.h"

namespace PhysicsRuntime {
    FPhysicsScene::FPhysicsScene(physx::PxPhysics& Physics, const physx::PxSceneDesc& Description)
		: mScene{Physics.createScene(Description)},
		mDefaultMaterial{mScene != nullptr ? Physics.createMaterial(0.5f, 0.5f, 0.0f) : nullptr} {
    }

    FPhysicsScene::~FPhysicsScene() {
        if (mScene != nullptr) {
            mScene->release();
        }

        if (mDefaultMaterial != nullptr) {
            mDefaultMaterial->release();
        }
    }

    bool FPhysicsScene::IsValid() const {
        return mScene != nullptr && mDefaultMaterial != nullptr;
    }

    std::unique_ptr<PhysicsRuntime::FBodyInstance> FPhysicsScene::CreateBody(PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose, const physx::PxGeometry& Geometry, const physx::PxTransform& ShapePose) {
        using namespace Core::Literals;

        if (!IsValid() || !Pose.isValid() || !ShapePose.isValid() || !physx::PxGeometryQuery::isValid(Geometry)) {
            return {};
        }

        auto Body{std::make_unique<PhysicsRuntime::FBodyInstance>(mScene->getPhysics(), BodyType, Pose)};
        physx::PxRigidActor* Actor{Body->GetActor()};

        if (Actor == nullptr) {
            return {};
        }

        physx::PxShape* Shape{physx::PxRigidActorExt::createExclusiveShape(*Actor, Geometry, *mDefaultMaterial)};

        if (Shape == nullptr) {
            return {};
        }

        Shape->setLocalPose(ShapePose);

        constexpr Core::FMass DefaultMass{1_kg};

        if (auto* RigidDynamic{Actor->is<physx::PxRigidDynamic>()}; RigidDynamic != nullptr && !physx::PxRigidBodyExt::setMassAndUpdateInertia(*RigidDynamic, DefaultMass / 1_kg)) {
            return {};
        }

        mScene->addActor(*Actor);

        if (Actor->getScene() != mScene) {
            return {};
        }

        return Body;
    }
}
