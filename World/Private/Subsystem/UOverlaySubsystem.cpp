#include "pch.h"
#include "World/Subsystem/UOverlaySubsystem.h"
#include "World/Component/UNameTagComponent.h"
#include "World/UWorld.h"

void UOverlaySubsystem::RegisterComponent(UNameTagComponent* Component) {
    if (Component != nullptr && std::ranges::find(mNameTags, Component) == mNameTags.end()) {
        mNameTags.push_back(Component);
    }
}

void UOverlaySubsystem::UnregisterComponent(UNameTagComponent* Component) {
    std::erase(mNameTags, Component);
}

void UOverlaySubsystem::BuildRenderProbes(FOverlayRenderData& Overlay, FObjectHandle SelectedActor) const {
    Overlay.mTextProbes.clear();
    Overlay.mSelectionProbes.clear();

    for (const UNameTagComponent* Component : mNameTags) {
        FOverlayTextProbe Probe{};

        if (Component != nullptr && Component->MakeOverlayText(Probe)) {
            Overlay.mTextProbes.push_back(std::move(Probe));
        }
    }

    if (GetWorld() == nullptr || !SelectedActor.IsValid()) {
        return;
    }

    for (const UStaticMeshComponent* Component : GetWorld()->GetRenderSubsystem().GetRegisteredComponents()) {
        if (Component == nullptr || !Component->IsRegistered() || !Component->IsVisible() || Component->GetOwner() == nullptr || Component->GetOwner()->GetHandle() != SelectedActor) {
            continue;
        }

        FActorProbe Probe{};

        Component->MakeRender(Probe);
        Probe.mOwnerHandle = SelectedActor;
        Overlay.mSelectionProbes.push_back(Probe);
    }
}

void UOverlaySubsystem::OnDeinitialize() {
    mNameTags.clear();
}
