#pragma once

#include "Engine/Component/ULightComponent.h"

class UDirectionalLightComponent final : public ULightComponent {
public:
    ReflectTypes(UDirectionalLightComponent, ULightComponent);

public:
    UDirectionalLightComponent();
    ~UDirectionalLightComponent() override;

public:
    ReflectClass(UDirectionalLightComponent);

    Uint32 GetCascadeCount() const;
    bool SetCascadeCount(Uint32 Count);
    float GetShadowDistance() const;
    bool SetShadowDistance(float Distance);
    float GetCascadeTransitionFraction() const;
    bool SetCascadeTransitionFraction(float Fraction);

    FLightSceneProxy CreateLightProxy() const override;

private:
    ReflectMembers();

private:
    Uint32 mCascadeCount{4};
    ReflectAccessor(mCascadeCount, GetCascadeCount, SetCascadeCount, "CascadeCount", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    float mShadowDistance{100.0f};
    ReflectAccessor(mShadowDistance, GetShadowDistance, SetShadowDistance, "ShadowDistance", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    float mCascadeTransitionFraction{0.1f};
    ReflectAccessor(mCascadeTransitionFraction, GetCascadeTransitionFraction, SetCascadeTransitionFraction, "CascadeTransitionFraction", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
