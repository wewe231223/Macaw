#include "pch.h"
#include "World/Subsystem/UOverlaySubsystem.h"
#include "World/UWorld.h"

void UOverlaySubsystem::BuildRenderProbes(FOverlayRenderData& Overlay, FObjectHandle SelectedActor) const {
    Overlay.mSelectionProbes.clear();

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
