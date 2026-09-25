#include "pch.h"

#include "URenderSubsystem.h"

#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "Scene/Component/UStaticMeshComponent.h"
#include "Scene/FWorldEditorContext.h"

#include "../../Render/Pipeline/UPipeline.h"

void URenderSubsystem::RegisterComponent(UStaticMeshComponent* Component) {
    if (Component == nullptr || ContainsComponent(Component)) {
        return;
    }

    mComponents.push_back(Component);
}

void URenderSubsystem::UnregisterComponent(UStaticMeshComponent* Component) {
    std::erase(mComponents, Component);
}

void URenderSubsystem::BuildRenderProbes(FAssetRegistry* AssetRegistry, FRenderProbe& Probe) const {
    Probe.mActorProbes.clear();
    Probe.mGizmoProbes.clear();

    const FWorldEditorContext* EditorContext{GetWorld()->GetEditorContext()};
    const AActor* SelectedActor{EditorContext != nullptr ? EditorContext->GetSelectedActor() : nullptr};
    for (const UStaticMeshComponent* Component : mComponents) {
        FActorProbe ActorProbe{};
        Component->MakeRender(ActorProbe);

        if (not Component->IsActive() or not Component->IsVisible())
            continue;

        if (AssetRegistry != nullptr && EditorContext != nullptr) {
            if (UPipeline * Pipeline{AssetRegistry->ResolveAsset<UPipeline>(Component->GetPipelineHandle())}) {
                Pipeline->SetRenderMode(static_cast<ERenderMode>(EditorContext->GetRenderModeState()));
            }
        }

        if (SelectedActor != nullptr && Component->GetOwner() == SelectedActor) {
            ActorProbe.mFlags |= static_cast<Uint32>(ERenderObjectFlags::Selected);
        }

        Probe.mActorProbes.push_back(ActorProbe);
    }
}

bool URenderSubsystem::ContainsComponent(const UStaticMeshComponent* Component) const {
    return std::ranges::find(mComponents, Component) != mComponents.end();
}

const TArray<UStaticMeshComponent*>& URenderSubsystem::GetRegisteredComponents() const {
    return mComponents;
}

void URenderSubsystem::OnDeinitialize() {
    mComponents.clear();
}
