#include "pch.h"

#include "ULocalLightComponent.h"

float ULocalLightComponent::GetAttenuationRadius() const {
    return mAttenuationRadius;
}

void ULocalLightComponent::SetAttenuationRadius(float InAttenuationRadius) {
    mAttenuationRadius = std::max(InAttenuationRadius, 0.0f);
}

void ULocalLightComponent::MakeLightProbe(FLightProbe& OutProbe) const {
    ULightComponent::MakeLightProbe(OutProbe);
    OutProbe.mAttenuationRadius = GetAttenuationRadius();
}

void ULocalLightComponent::Serialize(FArchive& Archive) {
    ULightComponent::Serialize(Archive);
    Archive.Serialize("AttenuationRadius", mAttenuationRadius);
}
