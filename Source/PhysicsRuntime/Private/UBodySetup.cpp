#include "PhysicsRuntime/UBodySetup.h"
#include "Core/Archive.h"
#include "CoreUObject/Reflection/UScriptStruct.h"

namespace PhysicsRuntime {
    namespace {
        void SerializeSetupStruct(Core::FArchive& Archive, std::string_view Name, const CoreUObject::UScriptStruct& Struct, void* Value) {
            if (!Archive.HasError() && (!Archive.IsLoading() || Archive.HasField(Name)) && Archive.BeginObject(Name)) {
                Struct.SerializeProperties(Value, Archive);
                Archive.EndObject();
            }
        }
    }

    void FAggregateGeom::Clear() {
        mSphereElems.clear();
        mBoxElems.clear();
        mCapsuleElems.clear();
        mConvexElems.clear();
    }

    UBodySetup::UBodySetup() = default;

    UBodySetup::~UBodySetup() = default;

    void UBodySetup::Serialize(Core::FArchive& Archive) {
        Super::Serialize(Archive);
        SerializeSetupStruct(Archive, "AggGeom", *PhysicsRuntime::FAggregateGeom::StaticStruct(), &mAggGeom);
        SerializeSetupStruct(Archive, "DefaultInstance", *PhysicsRuntime::FBodyInstanceSettings::StaticStruct(), &mDefaultInstance);
        SerializeSetupStruct(Archive, "WalkableSlopeOverride", *PhysicsRuntime::FWalkableSlopeOverride::StaticStruct(), &mWalkableSlopeOverride);
    }

    PhysicsRuntime::FAggregateGeom& UBodySetup::GetAggGeom() {
        return mAggGeom;
    }

    const PhysicsRuntime::FAggregateGeom& UBodySetup::GetAggGeom() const {
        return mAggGeom;
    }

    PhysicsRuntime::FBodyInstanceSettings& UBodySetup::GetDefaultInstance() {
        return mDefaultInstance;
    }

    const PhysicsRuntime::FBodyInstanceSettings& UBodySetup::GetDefaultInstance() const {
        return mDefaultInstance;
    }

    const CoreUObject::TObjectRef<PhysicsRuntime::UPhysicalMaterial>& UBodySetup::GetPhysMaterial() const {
        return mPhysMaterial;
    }

    void UBodySetup::SetPhysMaterial(const CoreUObject::TObjectRef<PhysicsRuntime::UPhysicalMaterial>& Material) {
        mPhysMaterial = Material;
    }

    PhysicsRuntime::FWalkableSlopeOverride& UBodySetup::GetWalkableSlopeOverride() {
        return mWalkableSlopeOverride;
    }

    const PhysicsRuntime::FWalkableSlopeOverride& UBodySetup::GetWalkableSlopeOverride() const {
        return mWalkableSlopeOverride;
    }

    bool UBodySetup::ShouldConsiderForBounds() const {
        return mConsiderForBounds;
    }

    void UBodySetup::SetConsiderForBounds(bool ConsiderForBounds) {
        mConsiderForBounds = ConsiderForBounds;
    }

    bool UBodySetup::IsDoubleSidedGeometry() const {
        return mDoubleSidedGeometry;
    }

    void UBodySetup::SetDoubleSidedGeometry(bool DoubleSidedGeometry) {
        mDoubleSidedGeometry = DoubleSidedGeometry;
    }
}
