#include "pch.h"
#include <cfloat>
#include "World/Component/ULocalLightComponent.h"

float ULocalLightComponent::GetAttenuationRadius() const {
    return mAttenuationRadius;
}

void ULocalLightComponent::SetAttenuationRadius(float InAttenuationRadius) {
    const float Radius{std::max(InAttenuationRadius, 0.0f)};

    if (mAttenuationRadius == Radius) {
        return;
    }

    mAttenuationRadius = Radius;
    MarkRenderStateDirty();
}

void ULocalLightComponent::BuildLightShaderParameters(FLightShaderParameters& OutParameters) const {
    ULightComponent::BuildLightShaderParameters(OutParameters);
    OutParameters.mAttenuationRadius = GetAttenuationRadius();
}

void ULocalLightComponent::Serialize(FArchive& Archive) {
    ULightComponent::Serialize(Archive);
    Archive.Serialize("AttenuationRadius", mAttenuationRadius);

    if (Archive.IsLoading()) {
        MarkRenderStateDirty();
    }
}
