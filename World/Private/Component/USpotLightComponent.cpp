#include "pch.h"
#include "World/Component/USpotLightComponent.h"

#include <numbers>

namespace {
    constexpr float MinimumConeAngle{0.0f};
    constexpr float MaximumConeAngle{89.9f};

    float ToRadians(float Degrees) {
        return Degrees * (std::numbers::pi_v<float> / 180.0f);
    }
}

ELightType USpotLightComponent::GetLightType() const {
    return ELightType::Spot;
}

float USpotLightComponent::GetInnerConeAngle() const {
    return mInnerConeAngle;
}

float USpotLightComponent::GetOuterConeAngle() const {
    return mOuterConeAngle;
}

void USpotLightComponent::SetInnerConeAngle(float InInnerConeAngle) {
    const float Angle{std::clamp(InInnerConeAngle, MinimumConeAngle, mOuterConeAngle)};

    if (mInnerConeAngle == Angle) {
        return;
    }

    mInnerConeAngle = Angle;
    MarkRenderStateDirty();
}

void USpotLightComponent::SetOuterConeAngle(float InOuterConeAngle) {
    const float Angle{std::clamp(InOuterConeAngle, mInnerConeAngle, MaximumConeAngle)};

    if (mOuterConeAngle == Angle) {
        return;
    }

    mOuterConeAngle = Angle;
    MarkRenderStateDirty();
}

void USpotLightComponent::BuildLightShaderParameters(FLightShaderParameters& OutParameters) const {
    Super::BuildLightShaderParameters(OutParameters);
    OutParameters.mInnerConeCos = std::cos(ToRadians(mInnerConeAngle));
    OutParameters.mOuterConeCos = std::cos(ToRadians(mOuterConeAngle));
}

void USpotLightComponent::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);
    StaticClass()->SerializeDeclaredProperties(this, Archive);

    if (Archive.IsLoading()) {
        SetInnerConeAngle(mInnerConeAngle);
        SetOuterConeAngle(mOuterConeAngle);
    }
}

USpotLightComponent::USpotLightComponent() {
    InitializeClass(StaticClass());
}
