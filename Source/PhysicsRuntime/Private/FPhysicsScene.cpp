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

    std::unique_ptr<PhysicsRuntime::FBodyInstance> FPhysicsScene::CreateBody(PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose, const PhysicsRuntime::UBodySetup& Setup, const PhysicsRuntime::UPhysicalMaterial* MaterialOverride) {
        using namespace Core::Literals;

        const PhysicsRuntime::FAggregateGeom& AggGeom{Setup.GetAggGeom()};

        if (!AggGeom.mConvexElems.empty() || (AggGeom.mSphereElems.empty() && AggGeom.mBoxElems.empty() && AggGeom.mCapsuleElems.empty())) {
            return {};
        }

        auto Body{CreateBodyActor(BodyType, Pose)};

        if (Body == nullptr) {
            return {};
        }

        const PhysicsRuntime::UPhysicalMaterial* Material{MaterialOverride != nullptr ? MaterialOverride : Setup.GetPhysMaterial().Get()};
        physx::PxMaterial* NativeMaterial{mRuntime.ResolveMaterial(Material != nullptr ? *Material : PhysicsRuntime::UPhysicalMaterial::GetDefault())};

        if (NativeMaterial == nullptr) {
            return {};
        }

        physx::PxRigidActor& Actor{*Body->GetActor()};

        for (const PhysicsRuntime::FBoxElem& Box : AggGeom.mBoxElems) {
            const FVector HalfExtent{Box.mDimensions * 0.5f};
            const physx::PxBoxGeometry Geometry{HalfExtent.x, HalfExtent.y, HalfExtent.z};

            if (!AttachShape(Actor, Geometry, MakeShapePose(Box.mCenter, Box.mRotation), *NativeMaterial)) {
                return {};
            }
        }

        for (const PhysicsRuntime::FSphereElem& Sphere : AggGeom.mSphereElems) {
            if (!AttachShape(Actor, physx::PxSphereGeometry{Sphere.mRadius}, MakeShapePose(Sphere.mCenter, FQuat{}), *NativeMaterial)) {
                return {};
            }
        }

        const physx::PxQuat CapsuleAxisRotation{-(90_deg / 1_rad), physx::PxVec3{0.0f, 1.0f, 0.0f}};

        for (const PhysicsRuntime::FCapsuleElem& Capsule : AggGeom.mCapsuleElems) {
            const physx::PxTransform ShapePose{MakeShapePose(Capsule.mCenter, Capsule.mRotation)};

            if (Capsule.mLength == 0.0f) {
                if (!AttachShape(Actor, physx::PxSphereGeometry{Capsule.mRadius}, ShapePose, *NativeMaterial)) {
                    return {};
                }
            } else {
                const physx::PxTransform CapsulePose{ShapePose.p, ShapePose.q * CapsuleAxisRotation};

                if (!AttachShape(Actor, physx::PxCapsuleGeometry{Capsule.mRadius, Capsule.mLength * 0.5f}, CapsulePose, *NativeMaterial)) {
                    return {};
                }
            }
        }

        if (!AddBodyToScene(Actor, Setup.GetDefaultInstance())) {
            return {};
        }

        return Body;
    }

    std::unique_ptr<PhysicsRuntime::FBodyInstance> FPhysicsScene::CreateBody(PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose, const PhysicsRuntime::UBodySetup& Setup, const PhysicsRuntime::UPhysicalMaterial& MaterialOverride) {
        return CreateBody(BodyType, Pose, Setup, &MaterialOverride);
    }

    std::unique_ptr<PhysicsRuntime::FBodyInstance> FPhysicsScene::CreateBody(PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose, const physx::PxGeometry& Geometry, const physx::PxTransform& ShapePose, const PhysicsRuntime::UPhysicalMaterial& Material) {
        auto Body{CreateBodyActor(BodyType, Pose)};

        if (Body == nullptr) {
            return {};
        }

        physx::PxMaterial* NativeMaterial{mRuntime.ResolveMaterial(Material)};

        if (NativeMaterial == nullptr || !AttachShape(*Body->GetActor(), Geometry, ShapePose, *NativeMaterial) || !AddBodyToScene(*Body->GetActor(), PhysicsRuntime::FBodyInstanceSettings{})) {
            return {};
        }

        return Body;
    }

    bool FPhysicsScene::SetBodyMaterial(const PhysicsRuntime::FBodyInstance& Body, const PhysicsRuntime::UPhysicalMaterial& Material) {
        physx::PxRigidActor* Actor{Body.GetActor()};

        if (!IsValid() || Actor == nullptr || Actor->getScene() != mScene) {
            return false;
        }

        const physx::PxU32 ShapeCount{Actor->getNbShapes()};

        if (ShapeCount == 0) {
            return false;
        }

        Core::TArray<physx::PxShape*> Shapes{};
        Shapes.resize(ShapeCount);
        physx::PxMaterial* Native{mRuntime.ResolveMaterial(Material)};

        if (Native == nullptr || Actor->getShapes(Shapes.data(), ShapeCount) != ShapeCount) {
            return false;
        }

        for (physx::PxShape* Shape : Shapes) {
            Shape->setMaterials(&Native, 1);
        }

        return true;
    }

    bool FPhysicsScene::Step(float DeltaSeconds) {
        if (mScene == nullptr || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0f) {
            return false;
        }

        mScene->simulate(DeltaSeconds);
        return mScene->fetchResults(true);
    }

    physx::PxTransform FPhysicsScene::MakeShapePose(const FVector& Center, const FQuat& Rotation) const {
        return physx::PxTransform{physx::PxVec3{Center.x, Center.y, Center.z}, physx::PxQuat{Rotation.x, Rotation.y, Rotation.z, Rotation.w}};
    }

    bool FPhysicsScene::AttachShape(physx::PxRigidActor& Actor, const physx::PxGeometry& Geometry, const physx::PxTransform& Pose, const physx::PxMaterial& Material) const {
        if (!Pose.isValid() || !physx::PxGeometryQuery::isValid(Geometry)) {
            return false;
        }

        physx::PxShape* Shape{physx::PxRigidActorExt::createExclusiveShape(Actor, Geometry, Material)};

        if (Shape == nullptr) {
            return false;
        }

        Shape->setLocalPose(Pose);
        return true;
    }

    std::unique_ptr<PhysicsRuntime::FBodyInstance> FPhysicsScene::CreateBodyActor(PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose) {
        if (!IsValid() || !Pose.isValid()) {
            return {};
        }

        auto Body{std::make_unique<PhysicsRuntime::FBodyInstance>(mScene->getPhysics(), BodyType, Pose)};

        if (Body->GetActor() == nullptr) {
            return {};
        }

        return Body;
    }

    bool FPhysicsScene::AddBodyToScene(physx::PxRigidActor& Actor, const PhysicsRuntime::FBodyInstanceSettings& Settings) {
        if (auto* Dynamic{Actor.is<physx::PxRigidDynamic>()}; Dynamic != nullptr) {
            if (!std::isfinite(Settings.mMassInKilograms) || Settings.mMassInKilograms <= 0.0f || !std::isfinite(Settings.mLinearDamping) || Settings.mLinearDamping < 0.0f || !std::isfinite(Settings.mAngularDamping) || Settings.mAngularDamping < 0.0f || !physx::PxRigidBodyExt::setMassAndUpdateInertia(*Dynamic, Settings.mMassInKilograms)) {
                return false;
            }

            Dynamic->setLinearDamping(Settings.mLinearDamping);
            Dynamic->setAngularDamping(Settings.mAngularDamping);
            Dynamic->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !Settings.mEnableGravity);
        }

        mScene->addActor(Actor);
        return Actor.getScene() == mScene;
    }
}
