#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "CoreUObject/UObject.h"
#include "UPhysicalMaterial.generated.h"

namespace PhysicsRuntime {
    UENUM()
    enum class EPhysicalMaterialCombineMode : Uint8 {
        Average,
        Min,
        Multiply,
        Max
    };

    UCLASS()
    class UPhysicalMaterial final : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        UPhysicalMaterial();
        ~UPhysicalMaterial() override;

    public:
        static const PhysicsRuntime::UPhysicalMaterial& GetDefault();

        float GetStaticFriction() const;
        bool SetStaticFriction(float Value);
        float GetDynamicFriction() const;
        bool SetDynamicFriction(float Value);
        float GetRestitution() const;
        bool SetRestitution(float Value);

        PhysicsRuntime::EPhysicalMaterialCombineMode GetFrictionCombineMode() const;
        bool SetFrictionCombineMode(PhysicsRuntime::EPhysicalMaterialCombineMode Value);
        PhysicsRuntime::EPhysicalMaterialCombineMode GetRestitutionCombineMode() const;
        bool SetRestitutionCombineMode(PhysicsRuntime::EPhysicalMaterialCombineMode Value);

        void CopySettingsFrom(const PhysicsRuntime::UPhysicalMaterial& Material);

    private:
        UPROPERTY(Edit, Serialize, Getter = GetStaticFriction, Setter = SetStaticFriction)
        float mStaticFriction{0.5f};

        UPROPERTY(Edit, Serialize, Getter = GetDynamicFriction, Setter = SetDynamicFriction)
        float mDynamicFriction{0.5f};

        UPROPERTY(Edit, Serialize, Getter = GetRestitution, Setter = SetRestitution)
        float mRestitution{};

        UPROPERTY(Edit, Serialize, Getter = GetFrictionCombineMode, Setter = SetFrictionCombineMode)
        PhysicsRuntime::EPhysicalMaterialCombineMode mFrictionCombineMode{PhysicsRuntime::EPhysicalMaterialCombineMode::Average};

        UPROPERTY(Edit, Serialize, Getter = GetRestitutionCombineMode, Setter = SetRestitutionCombineMode)
        PhysicsRuntime::EPhysicalMaterialCombineMode mRestitutionCombineMode{PhysicsRuntime::EPhysicalMaterialCombineMode::Average};
    };
}
