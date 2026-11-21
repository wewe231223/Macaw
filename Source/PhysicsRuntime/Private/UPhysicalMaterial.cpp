#include "PhysicsRuntime/UPhysicalMaterial.h"

#include <cmath>

namespace PhysicsRuntime {
    UPhysicalMaterial::UPhysicalMaterial() = default;
    UPhysicalMaterial::~UPhysicalMaterial() = default;

    const PhysicsRuntime::UPhysicalMaterial& UPhysicalMaterial::GetDefault() {
        static const PhysicsRuntime::UPhysicalMaterial Material{};

        return Material;
    }

    float UPhysicalMaterial::GetStaticFriction() const {
        return mStaticFriction;
    }

    bool UPhysicalMaterial::SetStaticFriction(float Value) {
        if (!std::isfinite(Value) || Value < 0.0f) {
            return false;
        }

        mStaticFriction = Value;

        return true;
    }

    float UPhysicalMaterial::GetDynamicFriction() const {
        return mDynamicFriction;
    }

    bool UPhysicalMaterial::SetDynamicFriction(float Value) {
        if (!std::isfinite(Value) || Value < 0.0f) {
            return false;
        }

        mDynamicFriction = Value;

        return true;
    }

    float UPhysicalMaterial::GetRestitution() const {
        return mRestitution;
    }

    bool UPhysicalMaterial::SetRestitution(float Value) {
        if (!(Value >= 0.0f && Value <= 1.0f)) {
            return false;
        }

        mRestitution = Value;

        return true;
    }

    PhysicsRuntime::EPhysicalMaterialCombineMode UPhysicalMaterial::GetFrictionCombineMode() const {
        return mFrictionCombineMode;
    }

    bool UPhysicalMaterial::SetFrictionCombineMode(PhysicsRuntime::EPhysicalMaterialCombineMode Value) {
        if (Value > PhysicsRuntime::EPhysicalMaterialCombineMode::Max) {
            return false;
        }

        mFrictionCombineMode = Value;

        return true;
    }

    PhysicsRuntime::EPhysicalMaterialCombineMode UPhysicalMaterial::GetRestitutionCombineMode() const {
        return mRestitutionCombineMode;
    }

    bool UPhysicalMaterial::SetRestitutionCombineMode(PhysicsRuntime::EPhysicalMaterialCombineMode Value) {
        if (Value > PhysicsRuntime::EPhysicalMaterialCombineMode::Max) {
            return false;
        }

        mRestitutionCombineMode = Value;

        return true;
    }

    void UPhysicalMaterial::CopySettingsFrom(const PhysicsRuntime::UPhysicalMaterial& Material) {
        mStaticFriction = Material.mStaticFriction;
        mDynamicFriction = Material.mDynamicFriction;
        mRestitution = Material.mRestitution;
        mFrictionCombineMode = Material.mFrictionCombineMode;
        mRestitutionCombineMode = Material.mRestitutionCombineMode;
    }
}
