#pragma once

#include "Core/Math/FQuat.h"
#include "Core/Math/FTransform.h"
#include "Core/Math/FVector.h"
#include "CoreUObject/TObjectRef.h"
#include "PhysicsRuntime/UBodySetupCore.h"
#include "PhysicsRuntime/UPhysicalMaterial.h"
#include "UBodySetup.generated.h"

namespace PhysicsRuntime {
    USTRUCT()
    struct FBoxElem {
        GENERATED_BODY()

    public:
        UPROPERTY(Edit, Serialize)
        FVector mCenter{};

        UPROPERTY(Edit, Serialize)
        FQuat mRotation{};

        UPROPERTY(Edit, Serialize)
        FVector mDimensions{};
    };

    USTRUCT()
    struct FSphereElem {
        GENERATED_BODY()

    public:
        UPROPERTY(Edit, Serialize)
        FVector mCenter{};

        UPROPERTY(Edit, Serialize)
        float mRadius{};
    };

    USTRUCT()
    struct FCapsuleElem {
        GENERATED_BODY()

    public:
        UPROPERTY(Edit, Serialize)
        FVector mCenter{};

        UPROPERTY(Edit, Serialize)
        FQuat mRotation{};

        UPROPERTY(Edit, Serialize)
        float mRadius{};

        UPROPERTY(Edit, Serialize)
        float mLength{};
    };

    USTRUCT()
    struct FConvexElem {
        GENERATED_BODY()

    public:
        UPROPERTY(Edit, Serialize)
        FTransform mTransform{};

        UPROPERTY(Edit, Serialize)
        Core::TArray<FVector> mVertices{};
    };

    USTRUCT()
    struct FAggregateGeom {
        GENERATED_BODY()

    public:
        void Clear();

    public:
        UPROPERTY(Edit, Serialize)
        Core::TArray<PhysicsRuntime::FSphereElem> mSphereElems{};

        UPROPERTY(Edit, Serialize)
        Core::TArray<PhysicsRuntime::FBoxElem> mBoxElems{};

        UPROPERTY(Edit, Serialize)
        Core::TArray<PhysicsRuntime::FCapsuleElem> mCapsuleElems{};

        UPROPERTY(Edit, Serialize)
        Core::TArray<PhysicsRuntime::FConvexElem> mConvexElems{};
    };

    USTRUCT()
    struct FBodyInstanceSettings {
        GENERATED_BODY()

    public:
        UPROPERTY(Edit, Serialize)
        float mMassInKilograms{1.0f};

        UPROPERTY(Edit, Serialize)
        float mLinearDamping{};

        UPROPERTY(Edit, Serialize)
        float mAngularDamping{0.05f};

        UPROPERTY(Edit, Serialize)
        bool mEnableGravity{true};
    };

    UENUM()
    enum class EWalkableSlopeBehavior : Uint8 {
        Default,
        Increase,
        Decrease,
        Unwalkable
    };

    USTRUCT()
    struct FWalkableSlopeOverride {
        GENERATED_BODY()

    public:
        UPROPERTY(Edit, Serialize)
        PhysicsRuntime::EWalkableSlopeBehavior mWalkableSlopeBehavior{PhysicsRuntime::EWalkableSlopeBehavior::Default};

        UPROPERTY(Edit, Serialize)
        float mWalkableSlopeAngle{};
    };

    UCLASS()
    class UBodySetup : public PhysicsRuntime::UBodySetupCore {
        GENERATED_BODY()

    public:
        UBodySetup();
        ~UBodySetup() override;

    public:
        void Serialize(Core::FArchive& Archive) override;

        PhysicsRuntime::FAggregateGeom& GetAggGeom();
        const PhysicsRuntime::FAggregateGeom& GetAggGeom() const;

        PhysicsRuntime::FBodyInstanceSettings& GetDefaultInstance();
        const PhysicsRuntime::FBodyInstanceSettings& GetDefaultInstance() const;

        const CoreUObject::TObjectRef<PhysicsRuntime::UPhysicalMaterial>& GetPhysMaterial() const;
        void SetPhysMaterial(const CoreUObject::TObjectRef<PhysicsRuntime::UPhysicalMaterial>& Material);

        PhysicsRuntime::FWalkableSlopeOverride& GetWalkableSlopeOverride();
        const PhysicsRuntime::FWalkableSlopeOverride& GetWalkableSlopeOverride() const;

        bool ShouldConsiderForBounds() const;
        void SetConsiderForBounds(bool ConsiderForBounds);
        bool IsDoubleSidedGeometry() const;
        void SetDoubleSidedGeometry(bool DoubleSidedGeometry);

    private:
        PhysicsRuntime::FAggregateGeom mAggGeom{};
        PhysicsRuntime::FBodyInstanceSettings mDefaultInstance{};
        PhysicsRuntime::FWalkableSlopeOverride mWalkableSlopeOverride{};

        UPROPERTY(Edit, Serialize)
        CoreUObject::TObjectRef<PhysicsRuntime::UPhysicalMaterial> mPhysMaterial{};

        UPROPERTY(Edit, Serialize)
        bool mConsiderForBounds{true};

        UPROPERTY(Edit, Serialize)
        bool mDoubleSidedGeometry{};
    };
}
