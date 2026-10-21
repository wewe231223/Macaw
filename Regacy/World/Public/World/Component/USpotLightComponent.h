#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/UPointLightComponent.h"

class USpotLightComponent : public UPointLightComponent {
public:
    ReflectTypes(USpotLightComponent, UPointLightComponent);

public:
    USpotLightComponent();
    ~USpotLightComponent() override = default;

public:
    ReflectClass(USpotLightComponent, EClassFlags::None);
    ReflectFunction(GetLightType);
    ELightType GetLightType() const override;
    ReflectFunction(GetInnerConeAngle);
    float GetInnerConeAngle() const;
    ReflectFunction(GetOuterConeAngle);
    float GetOuterConeAngle() const;
    ReflectFunction(SetInnerConeAngle, FName{"InInnerConeAngle"});
    void SetInnerConeAngle(float InInnerConeAngle);
    ReflectFunction(SetOuterConeAngle, FName{"InOuterConeAngle"});
    void SetOuterConeAngle(float InOuterConeAngle);

    void BuildLightShaderParameters(FLightShaderParameters& OutParameters) const override;

protected:
    void Serialize(FArchive& Archive) override;

private:
    ReflectAccessor(mInnerConeAngle, GetInnerConeAngle, SetInnerConeAngle, "InnerConeAngle", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mInnerConeAngle), ReflectEditor("Spot Light", "Inner Cone Angle"), ReflectRange(0.0f, 89.9f), ReflectBounds(nullptr, ReflectBound(GetOuterConeAngle)));
    float mInnerConeAngle{30.0f};
    ReflectAccessor(mOuterConeAngle, GetOuterConeAngle, SetOuterConeAngle, "OuterConeAngle", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mOuterConeAngle), ReflectEditor("Spot Light", "Outer Cone Angle"), ReflectRange(0.0f, 89.9f), ReflectBounds(ReflectBound(GetInnerConeAngle), nullptr));
    float mOuterConeAngle{45.0f};

    ReflectMembers();
};
