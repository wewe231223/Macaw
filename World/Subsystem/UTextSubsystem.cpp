#include "pch.h"

#include "UTextSubsystem.h"
#include "World/Component/UBillboardTextComponent.h"

void UTextSubsystem::RegisterComponent(UBillboardTextComponent* Component) {
    if (Component == nullptr || ContainsComponent(Component)) {
        return;
    }

    mComponents.emplace_back(Component);
}

void UTextSubsystem::UnregisterComponent(UBillboardTextComponent* Component) {
    std::erase(mComponents, Component);
}

void UTextSubsystem::BuildTextProbes(FRenderProbe& Probe) const {
    Probe.mTextProbes.clear();

    for (UBillboardTextComponent* Component : mComponents) {
        if (Component == nullptr) {
            continue;
        }

        FTextProbe TextProbe{};

        if (Component->MakeTextRender(TextProbe)) {
            Probe.mTextProbes.push_back(std::move(TextProbe));
        }
    }
}

bool UTextSubsystem::ContainsComponent(const UBillboardTextComponent* Component) const {
    return std::ranges::find(mComponents, Component) != mComponents.end();
}

const TArray<UBillboardTextComponent*>& UTextSubsystem::GetRegisteredComponents() const {
    return mComponents;
}

void UTextSubsystem::OnDeinitialize() {
    mComponents.clear();
}
