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
    Super::BuildLightShaderParameters(OutParameters);
    OutParameters.mAttenuationRadius = GetAttenuationRadius();
}

void ULocalLightComponent::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);
    StaticClass()->SerializeDeclaredProperties(this, Archive);

    if (Archive.IsLoading()) {
        MarkRenderStateDirty();
    }
}

ULocalLightComponent::ULocalLightComponent() {
    InitializeClass(StaticClass());
}
