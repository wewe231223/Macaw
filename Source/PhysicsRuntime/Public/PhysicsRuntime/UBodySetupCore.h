#pragma once

#include "Core/FName.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "CoreUObject/UObject.h"
#include "UBodySetupCore.generated.h"

namespace PhysicsRuntime {
    UENUM()
    enum class EPhysicsType : Uint8 {
        Default,
        Kinematic,
        Simulated
    };

    UENUM()
    enum class EBodyCollisionResponse : Uint8 {
        Enabled,
        Disabled
    };

    UENUM()
    enum class ECollisionTraceFlag : Uint8 {
        UseDefault,
        UseSimpleAndComplex,
        UseSimpleAsComplex,
        UseComplexAsSimple
    };

    UCLASS()
    class UBodySetupCore : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        UBodySetupCore();
        ~UBodySetupCore() override;

    public:
        void Serialize(Core::FArchive& Archive) override;

        const Core::FName& GetBoneName() const;
        void SetBoneName(const Core::FName& BoneName);

        PhysicsRuntime::EPhysicsType GetPhysicsType() const;
        void SetPhysicsType(PhysicsRuntime::EPhysicsType PhysicsType);
        PhysicsRuntime::EBodyCollisionResponse GetCollisionResponse() const;
        void SetCollisionResponse(PhysicsRuntime::EBodyCollisionResponse CollisionResponse);
        PhysicsRuntime::ECollisionTraceFlag GetCollisionTraceFlag() const;
        void SetCollisionTraceFlag(PhysicsRuntime::ECollisionTraceFlag CollisionTraceFlag);

    private:
        Core::FName mBoneName{};

        UPROPERTY(Edit, Serialize)
        PhysicsRuntime::EPhysicsType mPhysicsType{PhysicsRuntime::EPhysicsType::Default};

        UPROPERTY(Edit, Serialize)
        PhysicsRuntime::EBodyCollisionResponse mCollisionResponse{PhysicsRuntime::EBodyCollisionResponse::Enabled};

        UPROPERTY(Edit, Serialize)
        PhysicsRuntime::ECollisionTraceFlag mCollisionTraceFlag{PhysicsRuntime::ECollisionTraceFlag::UseDefault};
    };
}
