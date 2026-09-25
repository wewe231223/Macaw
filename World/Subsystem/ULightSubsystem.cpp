#include "pch.h"

#include "ULightSubsystem.h"

#include "World/Component/ULightComponent.h"

void ULightSubsystem::RegisterComponent(ULightComponent* Component) {
    if (Component == nullptr || ContainsComponent(Component)) {
        return;
    }

    mComponents.push_back(Component);
}

void ULightSubsystem::UnregisterComponent(ULightComponent* Component) {
    std::erase(mComponents, Component);
}

void ULightSubsystem::BuildLightProbes(FRenderProbe& Probe) const {
    Probe.mLightProbes.clear();
    Probe.mLightProbes.reserve(mComponents.size());

    for (const ULightComponent* Component : mComponents) {
        if (Component == nullptr || !Component->IsActive() || !Component->IsVisible()) {
            continue;
        }

        FLightProbe LightProbe{};
        Component->MakeLightProbe(LightProbe);
        Probe.mLightProbes.push_back(LightProbe);
    }
}

bool ULightSubsystem::ContainsComponent(const ULightComponent* Component) const {
    return std::ranges::find(mComponents, Component) != mComponents.end();
}

const TArray<ULightComponent*>& ULightSubsystem::GetRegisteredComponents() const {
    return mComponents;
}

void ULightSubsystem::OnDeinitialize() {
    mComponents.clear();
}
