#include "PhysicsRuntime/FPhysicsScene.h"
#include "PhysicsRuntime/FPhysicsRuntime.h"
#include "Core/Literals/UnitLiterals.h"

#include <cmath>

namespace PhysicsRuntime {
    FPhysicsScene::FPhysicsScene(PhysicsRuntime::FPhysicsRuntime& Runtime, physx::PxPhysics& Physics, const physx::PxSceneDesc& Description)
		: mRuntime{Runtime},
		  mScene{Physics.createScene(Description)} {
    }

    FPhysicsScene::~FPhysicsScene() {
        if (mScene != nullptr) {
            mScene->release();
        }
    }

    bool FPhysicsScene::IsValid() const {
        return mScene != nullptr;
    }

    std::unique_ptr<PhysicsRuntime::FBodyInstance> FPhysicsScene::CreateBody(PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose, const physx::PxGeometry& Geometry, const physx::PxTransform& ShapePose, const PhysicsRuntime::UPhysicalMaterial& Material) {
        using namespace Core::Literals;

        if (!IsValid() || !Pose.isValid() || !ShapePose.isValid() || !physx::PxGeometryQuery::isValid(Geometry)) {
            return {};
        }

        auto Body{std::make_unique<PhysicsRuntime::FBodyInstance>(mScene->getPhysics(), BodyType, Pose)};
        physx::PxRigidActor* Actor{Body->GetActor()};

        physx::PxMaterial* NativeMaterial{mRuntime.ResolveMaterial(Material)};

        if (Actor == nullptr || NativeMaterial == nullptr) {
            return {};
        }

        physx::PxShape* Shape{physx::PxRigidActorExt::createExclusiveShape(*Actor, Geometry, *NativeMaterial)};

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

    bool FPhysicsScene::SetBodyMaterial(const PhysicsRuntime::FBodyInstance& Body, const PhysicsRuntime::UPhysicalMaterial& Material) {
        physx::PxRigidActor* Actor{Body.GetActor()};

        if (!IsValid() || Actor == nullptr || Actor->getScene() != mScene) {
            return false;
        }

        physx::PxShape* Shape{};

        if (Actor->getShapes(&Shape, 1) != 1 || Shape == nullptr) {
            return false;
        }

        physx::PxMaterial* Native{mRuntime.ResolveMaterial(Material)};

        if (Native == nullptr) {
            return false;
        }

        Shape->setMaterials(&Native, 1);

        return true;
    }

    bool FPhysicsScene::Step(float DeltaSeconds) {
        if (mScene == nullptr || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0f) {
            return false;
        }

        mScene->simulate(DeltaSeconds);

        return mScene->fetchResults(true);
    }
}
