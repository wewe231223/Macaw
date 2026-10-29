#pragma once

#include "Engine/Component/ULightComponent.h"

class UPointLightComponent : public ULightComponent {
public:
    ReflectTypes(UPointLightComponent, ULightComponent);

public:
    UPointLightComponent();
    ~UPointLightComponent() override;

public:
    ReflectClass(UPointLightComponent);

    FLightSceneProxy CreateLightProxy() const override;

    float GetAttenuationRadius() const;
    bool SetAttenuationRadius(float AttenuationRadius);
    float GetLightFalloffExponent() const;
    bool SetLightFalloffExponent(float LightFalloffExponent);

private:
    ReflectMembers();

private:
    float mAttenuationRadius{10.0f};
    ReflectAccessor(mAttenuationRadius, GetAttenuationRadius, SetAttenuationRadius, "AttenuationRadius", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    float mLightFalloffExponent{2.0f};
    ReflectAccessor(mLightFalloffExponent, GetLightFalloffExponent, SetLightFalloffExponent, "LightFalloffExponent", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
