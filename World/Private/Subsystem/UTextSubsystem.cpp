#include "pch.h"
#include "World/Subsystem/UTextSubsystem.h"
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

void UTextSubsystem::BuildTextProbes(FSceneRenderData& Scene) const {
    Scene.mTextProbes.clear();

    for (UBillboardTextComponent* Component : mComponents) {
        if (Component == nullptr) {
            continue;
        }

        FTextProbe TextProbe{};

        if (Component->MakeTextRender(TextProbe)) {
            Scene.mTextProbes.push_back(std::move(TextProbe));
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

const FTypeInfo* UTextSubsystem::StaticTypeInfo() noexcept {
    static const FTypeInfo Information{"UTextSubsystem", UWorldSubsystem::StaticTypeInfo(), +[]() -> std::unique_ptr<UObject> {
        return std::make_unique<UTextSubsystem>();
    }};
    return &Information;
}

const FTypeInfo* UTextSubsystem::GetTypeInfo() const noexcept {
    return StaticTypeInfo();
}
