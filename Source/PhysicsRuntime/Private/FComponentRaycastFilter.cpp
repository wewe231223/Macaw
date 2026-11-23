#include "FComponentRaycastFilter.h"
#include <PxRigidActor.h>

namespace PhysicsRuntime {
    FComponentRaycastFilter::FComponentRaycastFilter(const Core::TMap<const physx::PxActor*, CoreUObject::FObjectHandle>& Components, const std::function<bool(CoreUObject::FObjectHandle)>& ShouldHit)
		: mComponents{Components},
		  mShouldHit{ShouldHit} {
    }

    physx::PxQueryHitType::Enum FComponentRaycastFilter::preFilter(const physx::PxFilterData&, const physx::PxShape*, const physx::PxRigidActor* Actor, physx::PxHitFlags&) {
        const auto Component{mComponents.find(Actor)};
        return Component != mComponents.end() && mShouldHit(Component->second) ? physx::PxQueryHitType::eBLOCK : physx::PxQueryHitType::eNONE;
    }

    physx::PxQueryHitType::Enum FComponentRaycastFilter::postFilter(const physx::PxFilterData&, const physx::PxQueryHit&) {
        return physx::PxQueryHitType::eBLOCK;
    }
}
