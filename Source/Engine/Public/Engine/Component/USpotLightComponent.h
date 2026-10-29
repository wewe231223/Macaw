#pragma once

#include "Engine/Component/UPointLightComponent.h"

class USpotLightComponent final : public UPointLightComponent {
public:
    ReflectTypes(USpotLightComponent, UPointLightComponent);

public:
    USpotLightComponent();
    ~USpotLightComponent() override;

public:
    ReflectClass(USpotLightComponent);

    FLightSceneProxy CreateLightProxy() const override;

    float GetInnerConeAngle() const;
    bool SetInnerConeAngle(float InnerConeAngle);
    float GetOuterConeAngle() const;
    bool SetOuterConeAngle(float OuterConeAngle);

private:
    ReflectMembers();

private:
    float mInnerConeAngle{20.0f};
    ReflectAccessor(mInnerConeAngle, GetInnerConeAngle, SetInnerConeAngle, "InnerConeAngle", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    float mOuterConeAngle{35.0f};
    ReflectAccessor(mOuterConeAngle, GetOuterConeAngle, SetOuterConeAngle, "OuterConeAngle", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
