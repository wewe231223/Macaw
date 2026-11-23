#include "PhysicsRuntime/FPhysicsScene.h"
#include "FComponentRaycastFilter.h"
#include "PhysicsRuntime/FPhysicsRuntime.h"
#include "Core/Literals/UnitLiterals.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace PhysicsRuntime {
    FPhysicsScene::FPhysicsScene(PhysicsRuntime::FPhysicsRuntime& Runtime, physx::PxPhysics& Physics, const physx::PxSceneDesc& Description)
		: mRuntime{Runtime} {
        physx::PxSceneDesc SceneDescription{Description};
        SceneDescription.simulationEventCallback = this;
        SceneDescription.filterShader = FilterCollisionPair;
        SceneDescription.kineKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
        SceneDescription.staticKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
        mScene = Physics.createScene(SceneDescription);
    }

    FPhysicsScene::~FPhysicsScene() {
        if (mScene != nullptr) {
            mScene->release();
        }
    }

    bool FPhysicsScene::IsValid() const {
        return mScene != nullptr;
    }

    std::unique_ptr<PhysicsRuntime::FBodyInstance> FPhysicsScene::CreateBody(PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose, const PhysicsRuntime::UBodySetup& Setup, const PhysicsRuntime::UPhysicalMaterial* MaterialOverride, const PhysicsRuntime::FPhysicsBodyCollision& Collision) {
        using namespace Core::Literals;

        if (Collision.mComponent.IsValid() != Collision.mOwner.IsValid()) {
            return {};
        }

        auto Body{CreateBodyActor(BodyType, Pose)};
        if (Body == nullptr) {
            return {};
        }

        physx::PxRigidActor& Actor{*Body->GetActor()};
        if (Collision.mCollisionEnabled == ECollisionEnabled::NoCollision) {
            if (!AddBodyToScene(Actor, Setup.GetDefaultInstance())) {
                return {};
            }

            return Body;
        }

        const bool HasQueries{PhysicsRuntime::HasQueryCollision(Collision.mCollisionEnabled)};
        const bool HasPhysics{PhysicsRuntime::HasPhysicsCollision(Collision.mCollisionEnabled)};
        if (!HasQueries && !HasPhysics) {
            return {};
        }

        const PhysicsRuntime::FAggregateGeom& AggGeom{Setup.GetAggGeom()};

        if (!AggGeom.mConvexElems.empty() || (AggGeom.mSphereElems.empty() && AggGeom.mBoxElems.empty() && AggGeom.mCapsuleElems.empty())) {
            return {};
        }

        const bool ReportsCollision{Collision.mComponent.IsValid()};
        const bool GeneratesOverlaps{ReportsCollision && HasQueries && Collision.mGenerateOverlapEvents};

        const PhysicsRuntime::UPhysicalMaterial* Material{MaterialOverride != nullptr ? MaterialOverride : Setup.GetPhysMaterial().Get()};
        physx::PxMaterial* NativeMaterial{mRuntime.ResolveMaterial(Material != nullptr ? *Material : PhysicsRuntime::UPhysicalMaterial::GetDefault())};

        if (NativeMaterial == nullptr) {
            return {};
        }

        for (const PhysicsRuntime::FBoxElem& Box : AggGeom.mBoxElems) {
            const FVector HalfExtent{Box.mDimensions * 0.5f};
            const physx::PxBoxGeometry Geometry{HalfExtent.x, HalfExtent.y, HalfExtent.z};

            if (!AttachShape(Actor, Geometry, MakeShapePose(Box.mCenter, Box.mRotation), *NativeMaterial, Collision)) {
                return {};
            }
        }

        for (const PhysicsRuntime::FSphereElem& Sphere : AggGeom.mSphereElems) {
            if (!AttachShape(Actor, physx::PxSphereGeometry{Sphere.mRadius}, MakeShapePose(Sphere.mCenter, FQuat{}), *NativeMaterial, Collision)) {
                return {};
            }
        }

        const physx::PxQuat CapsuleAxisRotation{-(90_deg / 1_rad), physx::PxVec3{0.0f, 1.0f, 0.0f}};

        for (const PhysicsRuntime::FCapsuleElem& Capsule : AggGeom.mCapsuleElems) {
            const physx::PxTransform ShapePose{MakeShapePose(Capsule.mCenter, Capsule.mRotation)};

            if (Capsule.mLength == 0.0f) {
                if (!AttachShape(Actor, physx::PxSphereGeometry{Capsule.mRadius}, ShapePose, *NativeMaterial, Collision)) {
                    return {};
                }
            } else {
                const physx::PxTransform CapsulePose{ShapePose.p, ShapePose.q * CapsuleAxisRotation};

                if (!AttachShape(Actor, physx::PxCapsuleGeometry{Capsule.mRadius, Capsule.mLength * 0.5f}, CapsulePose, *NativeMaterial, Collision)) {
                    return {};
                }
            }
        }

        if (!AddBodyToScene(Actor, Setup.GetDefaultInstance(), !HasPhysics && !GeneratesOverlaps)) {
            return {};
        }

        if (ReportsCollision) {
            mCollisionComponents.emplace(&Actor, Collision.mComponent);
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

    void FPhysicsScene::RemoveCollisionBody(const PhysicsRuntime::FBodyInstance& Body) {
        const auto Component{mCollisionComponents.find(Body.GetActor())};
        if (Component == mCollisionComponents.end()) {
            return;
        }

        const auto Handle{Component->second};
        mCollisionComponents.erase(Component);

        if (const auto Trigger{mTriggerOverlaps.find(Handle)}; Trigger != mTriggerOverlaps.end()) {
            for (const auto& Other : Trigger->second) {
                mCollisionEvents.push_back({EPhysicsCollisionEvent::EndOverlap, Handle, Other.first});
            }

            mTriggerOverlaps.erase(Trigger);
        }

        for (auto Trigger{mTriggerOverlaps.begin()}; Trigger != mTriggerOverlaps.end();) {
            if (Trigger->second.erase(Handle) != 0) {
                mCollisionEvents.push_back({EPhysicsCollisionEvent::EndOverlap, Trigger->first, Handle});
            }

            if (Trigger->second.empty()) {
                Trigger = mTriggerOverlaps.erase(Trigger);
            } else {
                ++Trigger;
            }
        }
    }

    bool FPhysicsScene::Step(float DeltaSeconds) {
        if (mScene == nullptr || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0f) {
            return false;
        }

        mScene->simulate(DeltaSeconds);
        return mScene->fetchResults(true);
    }

    Core::TArray<PhysicsRuntime::FPhysicsCollisionEvent> FPhysicsScene::TakeCollisionEvents() {
        return std::exchange(mCollisionEvents, {});
    }

    bool FPhysicsScene::Raycast(const FVector& Origin, const FVector& Direction, const std::function<bool(CoreUObject::FObjectHandle)>& ShouldHit, float& Distance, CoreUObject::FObjectHandle& Component) const {
        if (mScene == nullptr) {
            return false;
        }

        FComponentRaycastFilter Filter{mCollisionComponents, ShouldHit};
        const physx::PxQueryFilterData FilterData{physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER};
        physx::PxRaycastBuffer Hit{};
        const physx::PxVec3 RayOrigin{Origin.x, Origin.y, Origin.z};
        const physx::PxVec3 RayDirection{Direction.x, Direction.y, Direction.z};
        if (!mScene->raycast(RayOrigin, RayDirection, Distance, Hit, physx::PxHitFlag::eDEFAULT, FilterData, &Filter) || !Hit.hasBlock) {
            return false;
        }

        Distance = Hit.block.distance;
        Component = mCollisionComponents.at(Hit.block.actor);

        return true;
    }

    physx::PxTransform FPhysicsScene::MakeShapePose(const FVector& Center, const FQuat& Rotation) const {
        return physx::PxTransform{physx::PxVec3{Center.x, Center.y, Center.z}, physx::PxQuat{Rotation.x, Rotation.y, Rotation.z, Rotation.w}};
    }

    bool FPhysicsScene::AttachShape(physx::PxRigidActor& Actor, const physx::PxGeometry& Geometry, const physx::PxTransform& Pose, const physx::PxMaterial& Material, const PhysicsRuntime::FPhysicsBodyCollision& Collision) const {
        if (!Pose.isValid() || !physx::PxGeometryQuery::isValid(Geometry)) {
            return false;
        }

        const bool HasPhysics{PhysicsRuntime::HasPhysicsCollision(Collision.mCollisionEnabled)};
        const bool HasQueries{PhysicsRuntime::HasQueryCollision(Collision.mCollisionEnabled)};
        const bool GeneratesOverlaps{HasQueries && Collision.mGenerateOverlapEvents && Collision.mComponent.IsValid()};
        physx::PxShapeFlags ShapeFlags{physx::PxShapeFlag::eVISUALIZATION};
        if (HasPhysics || GeneratesOverlaps) {
            ShapeFlags |= physx::PxShapeFlag::eSIMULATION_SHAPE;
        }

        if (HasQueries) {
            ShapeFlags |= physx::PxShapeFlag::eSCENE_QUERY_SHAPE;
        }

        physx::PxShape* Shape{physx::PxRigidActorExt::createExclusiveShape(Actor, Geometry, Material, ShapeFlags)};

        if (Shape == nullptr) {
            return false;
        }

        Shape->setLocalPose(Pose);

        if (Collision.mComponent.IsValid() && Collision.mOwner.IsValid()) {
            const physx::PxU32 OwnerId{static_cast<physx::PxU32>(Collision.mOwner.mIndex) + 1U};
            Shape->setSimulationFilterData(physx::PxFilterData{OwnerId, Collision.mOwner.mGeneration, HasPhysics ? 0U : TriggerTargetOnly, GeneratesOverlaps ? GenerateOverlapEventsFlag : 0U});

            if (!GeneratesOverlaps) {
                return true;
            }

            physx::PxShape* Trigger{physx::PxRigidActorExt::createExclusiveShape(Actor, Geometry, Material, physx::PxShapeFlag::eTRIGGER_SHAPE)};
            if (Trigger == nullptr) {
                return false;
            }

            Trigger->setLocalPose(Pose);
            Trigger->setSimulationFilterData(physx::PxFilterData{OwnerId, Collision.mOwner.mGeneration, 0U, GenerateOverlapEventsFlag});
        }

        return true;
    }

    FVector FPhysicsScene::ToVector(const physx::PxVec3& Value) const {
        return FVector{Value.x, Value.y, Value.z};
    }

    physx::PxFilterFlags FPhysicsScene::FilterCollisionPair(physx::PxFilterObjectAttributes FirstAttributes, physx::PxFilterData FirstData, physx::PxFilterObjectAttributes SecondAttributes, physx::PxFilterData SecondData, physx::PxPairFlags& PairFlags, const void* Data, physx::PxU32 Size) {
        using namespace physx;

        if (FirstData.word0 == 0U && SecondData.word0 == 0U) {
            return PxDefaultSimulationFilterShader(FirstAttributes, FirstData, SecondAttributes, SecondData, PairFlags, Data, Size);
        }

        const bool FirstTrigger{PxFilterObjectIsTrigger(FirstAttributes)};
        const bool SecondTrigger{PxFilterObjectIsTrigger(SecondAttributes)};
        const bool BothRegistered{FirstData.word0 != 0U && SecondData.word0 != 0U};
        const bool SameOwner{BothRegistered && FirstData.word0 == SecondData.word0 && FirstData.word1 == SecondData.word1};

        if (FirstTrigger || SecondTrigger) {
            const physx::PxU32 TriggerOwner{FirstTrigger ? FirstData.word0 : SecondData.word0};
            const physx::PxU32 TargetOwner{FirstTrigger ? SecondData.word0 : FirstData.word0};
            if (!BothRegistered || SameOwner || FirstTrigger == SecondTrigger || TriggerOwner > TargetOwner || FirstData.word3 != GenerateOverlapEventsFlag || SecondData.word3 != GenerateOverlapEventsFlag) {
                return PxFilterFlag::eSUPPRESS;
            }

            PairFlags = PxPairFlag::eTRIGGER_DEFAULT;
            return PxFilterFlag::eDEFAULT;
        }

        if (FirstData.word2 == TriggerTargetOnly || SecondData.word2 == TriggerTargetOnly) {
            return PxFilterFlag::eSUPPRESS;
        }

        PairFlags = PxPairFlag::eCONTACT_DEFAULT;
        if (BothRegistered && !SameOwner) {
            PairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_CONTACT_POINTS;
        }

        return PxFilterFlag::eDEFAULT;
    }

    void FPhysicsScene::onContact(const physx::PxContactPairHeader& Header, const physx::PxContactPair* Pairs, physx::PxU32 Count) {
        if (Header.flags.isSet(physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_0) || Header.flags.isSet(physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_1)) {
            return;
        }

        const auto First{mCollisionComponents.find(Header.actors[0])};
        const auto Second{mCollisionComponents.find(Header.actors[1])};
        if (First == mCollisionComponents.end() || Second == mCollisionComponents.end()) {
            return;
        }

        Core::TArray<physx::PxContactPairPoint> Contacts{};
        float MaxImpulseSquared{-1.0f};
        physx::PxContactPairPoint Selected{};

        for (physx::PxU32 Index{}; Index < Count; ++Index) {
            const physx::PxContactPair& Pair{Pairs[Index]};
            if (!Pair.events.isSet(physx::PxPairFlag::eNOTIFY_TOUCH_FOUND) || Pair.flags.isSet(physx::PxContactPairFlag::eREMOVED_SHAPE_0) || Pair.flags.isSet(physx::PxContactPairFlag::eREMOVED_SHAPE_1)) {
                continue;
            }

            Contacts.resize(Pair.contactCount);
            const physx::PxU32 ContactCount{Pair.extractContacts(Contacts.data(), Pair.contactCount)};
            for (physx::PxU32 ContactIndex{}; ContactIndex < ContactCount; ++ContactIndex) {
                const physx::PxContactPairPoint& Point{Contacts[ContactIndex]};
                const float ImpulseSquared{Point.impulse.magnitudeSquared()};
                if (ImpulseSquared > MaxImpulseSquared) {
                    Selected = Point;
                    MaxImpulseSquared = ImpulseSquared;
                }
            }
        }

        if (MaxImpulseSquared >= 0.0f) {
            mCollisionEvents.push_back({EPhysicsCollisionEvent::Hit, First->second, Second->second, ToVector(Selected.position), ToVector(Selected.normal), ToVector(Selected.impulse), std::max(0.0f, -Selected.separation)});
        }
    }

    void FPhysicsScene::onTrigger(physx::PxTriggerPair* Pairs, physx::PxU32 Count) {
        for (physx::PxU32 Index{}; Index < Count; ++Index) {
            const physx::PxTriggerPair& Pair{Pairs[Index]};
            if (Pair.flags.isSet(physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER) || Pair.flags.isSet(physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER)) {
                continue;
            }

            const auto First{mCollisionComponents.find(Pair.triggerActor)};
            const auto Second{mCollisionComponents.find(Pair.otherActor)};
            if (First == mCollisionComponents.end() || Second == mCollisionComponents.end()) {
                continue;
            }

            if (Pair.status == physx::PxPairFlag::eNOTIFY_TOUCH_FOUND) {
                Uint32& ActiveShapes{mTriggerOverlaps[First->second][Second->second]};
                if (ActiveShapes++ == 0) {
                    mCollisionEvents.push_back({EPhysicsCollisionEvent::BeginOverlap, First->second, Second->second});
                }
            } else if (const auto Trigger{mTriggerOverlaps.find(First->second)}; Trigger != mTriggerOverlaps.end()) {
                auto& Overlaps{Trigger->second};
                const auto Other{Overlaps.find(Second->second)};
                if (Other != Overlaps.end() && --Other->second == 0) {
                    mCollisionEvents.push_back({EPhysicsCollisionEvent::EndOverlap, First->second, Second->second});
                    Overlaps.erase(Other);
                    if (Overlaps.empty()) {
                        mTriggerOverlaps.erase(Trigger);
                    }
                }
            }
        }
    }

    void FPhysicsScene::onConstraintBreak(physx::PxConstraintInfo*, physx::PxU32) {
    }

    void FPhysicsScene::onWake(physx::PxActor**, physx::PxU32) {
    }

    void FPhysicsScene::onSleep(physx::PxActor**, physx::PxU32) {
    }

    void FPhysicsScene::onAdvance(const physx::PxRigidBody* const*, const physx::PxTransform*, physx::PxU32) {
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

    bool FPhysicsScene::AddBodyToScene(physx::PxRigidActor& Actor, const PhysicsRuntime::FBodyInstanceSettings& Settings, bool IncludeQueryShapes) {
        if (auto* Dynamic{Actor.is<physx::PxRigidDynamic>()}; Dynamic != nullptr) {
            if (!std::isfinite(Settings.mMassInKilograms) || Settings.mMassInKilograms <= 0.0f || !std::isfinite(Settings.mLinearDamping) || Settings.mLinearDamping < 0.0f || !std::isfinite(Settings.mAngularDamping) || Settings.mAngularDamping < 0.0f) {
                return false;
            }

            if (Actor.getNbShapes() == 0) {
                Dynamic->setMass(Settings.mMassInKilograms);
            } else if (!physx::PxRigidBodyExt::setMassAndUpdateInertia(*Dynamic, Settings.mMassInKilograms, nullptr, IncludeQueryShapes)) {
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
