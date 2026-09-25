#include "pch.h"

#include "ULocalLightComponent.h"

#include "Render/Panel/FPropertyEditorContext.h"

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

void ULocalLightComponent::DrawPanels(FPropertyEditorContext& Context) {
    ULightComponent::DrawPanels(Context);

    if (!Context.BeginCategory("Local Light")) {
        return;
    }

    Context.DrawFloat("Attenuation Radius", GetAttenuationRadius(), 1.0f, 0.0f, FLT_MAX, [this](float InAttenuationRadius) {
        SetAttenuationRadius(InAttenuationRadius);
    });
}

void ULocalLightComponent::Serialize(FArchive& Archive) {
    ULightComponent::Serialize(Archive);
    Archive.Serialize("AttenuationRadius", mAttenuationRadius);
}
