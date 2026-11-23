#pragma once

#include "CoreUObject/FObjectHandle.h"
#include <PxActor.h>
#include <PxQueryFiltering.h>
#include <functional>

namespace PhysicsRuntime {
    class FComponentRaycastFilter : public physx::PxQueryFilterCallback {
    public:
        FComponentRaycastFilter(const Core::TMap<const physx::PxActor*, CoreUObject::FObjectHandle>& Components, const std::function<bool(CoreUObject::FObjectHandle)>& ShouldHit);

    public:
        physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData&, const physx::PxShape*, const physx::PxRigidActor* Actor, physx::PxHitFlags&) override;
        physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData&, const physx::PxQueryHit&) override;

    private:
        const Core::TMap<const physx::PxActor*, CoreUObject::FObjectHandle>& mComponents;
        const std::function<bool(CoreUObject::FObjectHandle)>& mShouldHit;
    };
}
