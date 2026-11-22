#include "PhysicsRuntime/UBodySetupCore.h"
#include "Core/Archive.h"

namespace PhysicsRuntime {
    UBodySetupCore::UBodySetupCore() = default;

    UBodySetupCore::~UBodySetupCore() = default;

    void UBodySetupCore::Serialize(Core::FArchive& Archive) {
        Super::Serialize(Archive);

        if (Archive.HasError() || (Archive.IsLoading() && !Archive.HasField("BoneName"))) {
            return;
        }

        Core::FString BoneName{mBoneName.ToString()};
        Archive.Serialize("BoneName", BoneName);

        if (Archive.IsLoading() && !Archive.HasError()) {
            mBoneName = Core::FName{BoneName};
        }
    }

    const Core::FName& UBodySetupCore::GetBoneName() const {
        return mBoneName;
    }

    void UBodySetupCore::SetBoneName(const Core::FName& BoneName) {
        mBoneName = BoneName;
    }

    PhysicsRuntime::EPhysicsType UBodySetupCore::GetPhysicsType() const {
        return mPhysicsType;
    }

    void UBodySetupCore::SetPhysicsType(PhysicsRuntime::EPhysicsType PhysicsType) {
        mPhysicsType = PhysicsType;
    }

    PhysicsRuntime::EBodyCollisionResponse UBodySetupCore::GetCollisionResponse() const {
        return mCollisionResponse;
    }

    void UBodySetupCore::SetCollisionResponse(PhysicsRuntime::EBodyCollisionResponse CollisionResponse) {
        mCollisionResponse = CollisionResponse;
    }

    PhysicsRuntime::ECollisionTraceFlag UBodySetupCore::GetCollisionTraceFlag() const {
        return mCollisionTraceFlag;
    }

    void UBodySetupCore::SetCollisionTraceFlag(PhysicsRuntime::ECollisionTraceFlag CollisionTraceFlag) {
        mCollisionTraceFlag = CollisionTraceFlag;
    }
}
