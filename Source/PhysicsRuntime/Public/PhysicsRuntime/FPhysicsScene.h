#pragma once

#include "CoreUObject/FObjectHandle.h"
#include "PhysicsRuntime/EBodyType.h"
#include "PhysicsRuntime/ECollisionEnabled.h"
#include "PhysicsRuntime/FBodyInstance.h"
#include "PhysicsRuntime/UBodySetup.h"
#include "PhysicsRuntime/UPhysicalMaterial.h"
#include <PxPhysicsAPI.h>
#include <functional>
#include <memory>

namespace PhysicsRuntime {
    class FPhysicsRuntime;

    struct FPhysicsBodyCollision {
        CoreUObject::FObjectHandle mComponent{};
        CoreUObject::FObjectHandle mOwner{};
        PhysicsRuntime::ECollisionEnabled mCollisionEnabled{PhysicsRuntime::ECollisionEnabled::QueryAndPhysics};
        bool mGenerateOverlapEvents{};
    };

    enum class EPhysicsCollisionEvent : Uint8 {
        Hit,
        BeginOverlap,
        EndOverlap
    };

    struct FPhysicsCollisionEvent {
        PhysicsRuntime::EPhysicsCollisionEvent mType{};
        CoreUObject::FObjectHandle mFirst{};
        CoreUObject::FObjectHandle mSecond{};
        FVector mPoint{};
        FVector mNormal{};
        FVector mNormalImpulse{};
        float mPenetrationDepth{};
    };

    class FPhysicsScene : private physx::PxSimulationEventCallback {
    private:
        using FTriggerOverlaps = CoreUObject::ObjectHashMap<CoreUObject::FObjectHandle, Uint32>;

        enum : physx::PxU32 {
            TriggerTargetOnly = 1,
            GenerateOverlapEventsFlag = 1
        };

    public:
        FPhysicsScene(PhysicsRuntime::FPhysicsRuntime& Runtime, physx::PxPhysics& Physics, const physx::PxSceneDesc& Description);
        ~FPhysicsScene() override;
        FPhysicsScene(const FPhysicsScene&) = delete;
        FPhysicsScene& operator=(const FPhysicsScene&) = delete;

    public:
        bool IsValid() const;

        std::unique_ptr<PhysicsRuntime::FBodyInstance> CreateBody(PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose, const PhysicsRuntime::UBodySetup& Setup, const PhysicsRuntime::UPhysicalMaterial* MaterialOverride = nullptr, const PhysicsRuntime::FPhysicsBodyCollision& Collision = {});
        std::unique_ptr<PhysicsRuntime::FBodyInstance> CreateBody(PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose, const PhysicsRuntime::UBodySetup& Setup, const PhysicsRuntime::UPhysicalMaterial& MaterialOverride);
        std::unique_ptr<PhysicsRuntime::FBodyInstance> CreateBody(PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose, const physx::PxGeometry& Geometry, const physx::PxTransform& ShapePose = physx::PxTransform{physx::PxIdentity}, const PhysicsRuntime::UPhysicalMaterial& Material = PhysicsRuntime::UPhysicalMaterial::GetDefault());
        bool SetBodyMaterial(const PhysicsRuntime::FBodyInstance& Body, const PhysicsRuntime::UPhysicalMaterial& Material);
        void RemoveCollisionBody(const PhysicsRuntime::FBodyInstance& Body);

        bool Step(float DeltaSeconds);
        Core::TArray<PhysicsRuntime::FPhysicsCollisionEvent> TakeCollisionEvents();
        bool Raycast(const FVector& Origin, const FVector& Direction, const std::function<bool(CoreUObject::FObjectHandle)>& ShouldHit, float& Distance, CoreUObject::FObjectHandle& Component) const;

    private:
        std::unique_ptr<PhysicsRuntime::FBodyInstance> CreateBodyActor(PhysicsRuntime::EBodyType BodyType, const physx::PxTransform& Pose);
        bool AddBodyToScene(physx::PxRigidActor& Actor, const PhysicsRuntime::FBodyInstanceSettings& Settings, bool IncludeQueryShapes = false);

        physx::PxTransform MakeShapePose(const FVector& Center, const FQuat& Rotation) const;
        bool AttachShape(physx::PxRigidActor& Actor, const physx::PxGeometry& Geometry, const physx::PxTransform& Pose, const physx::PxMaterial& Material, const PhysicsRuntime::FPhysicsBodyCollision& Collision = {}) const;
        FVector ToVector(const physx::PxVec3& Value) const;

        static physx::PxFilterFlags FilterCollisionPair(physx::PxFilterObjectAttributes FirstAttributes, physx::PxFilterData FirstData, physx::PxFilterObjectAttributes SecondAttributes, physx::PxFilterData SecondData, physx::PxPairFlags& PairFlags, const void*, physx::PxU32);

        void onContact(const physx::PxContactPairHeader& Header, const physx::PxContactPair* Pairs, physx::PxU32 Count) override;
        void onTrigger(physx::PxTriggerPair* Pairs, physx::PxU32 Count) override;

        void onConstraintBreak(physx::PxConstraintInfo*, physx::PxU32) override;
        void onWake(physx::PxActor**, physx::PxU32) override;
        void onSleep(physx::PxActor**, physx::PxU32) override;
        void onAdvance(const physx::PxRigidBody* const*, const physx::PxTransform*, physx::PxU32) override;

    private:
        PhysicsRuntime::FPhysicsRuntime& mRuntime;
        physx::PxScene* mScene{};
        Core::TMap<const physx::PxActor*, CoreUObject::FObjectHandle> mCollisionComponents{};
        CoreUObject::ObjectHashMap<CoreUObject::FObjectHandle, FTriggerOverlaps> mTriggerOverlaps{};
        Core::TArray<PhysicsRuntime::FPhysicsCollisionEvent> mCollisionEvents{};
    };
}
