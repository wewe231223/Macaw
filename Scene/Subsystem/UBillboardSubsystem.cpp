#include "pch.h"
#include "UBillboardSubsystem.h"

#include "Scene/AActor.h"
#include "Scene/UWorld.h"
#include "Scene/Component/UBillboardComponent.h"
#include "Scene/FWorldEditorContext.h"

#include "../../Render/Pipeline/UPipeline.h"

void UBillboardSubsystem::RegisterComponent(UBillboardComponent* Component) {
    if (Component == nullptr || ContainsComponent(Component)) {
        return;
    }

    mComponents.push_back(Component);
}

void UBillboardSubsystem::UnregisterComponent(UBillboardComponent* Component) {
    std::erase(mComponents, Component);
}

void UBillboardSubsystem::BuildRenderProbes(FAssetRegistry* AssetRegistry, FRenderProbe& Probe) const {
    Probe.mBillboardProbes.clear();

    const FWorldEditorContext* EditorContext{GetWorld()->GetEditorContext()};
    const AActor* SelectedActor{EditorContext != nullptr ? EditorContext->GetSelectedActor() : nullptr};
    for (const UBillboardComponent* Component : mComponents) {
        FBillboardProbe BillboardProbe{};
        if (!Component->MakeBillboardRender(BillboardProbe))
            continue;

        if (not Component->IsActive() or not Component->IsVisible())
            continue;

        if (AssetRegistry != nullptr) {
            if (UPipeline * Pipeline{AssetRegistry->ResolveAsset<UPipeline>(Component->GetPipelineHandle())}) {
                Pipeline->SetRenderMode(static_cast<ERenderMode>(EditorContext->GetRenderModeState()));
            }
        }

        Probe.mBillboardProbes.push_back(BillboardProbe);
    }
}

bool UBillboardSubsystem::ContainsComponent(const UBillboardComponent* Component) {
    return std::ranges::find(mComponents, Component) != mComponents.end();
}

const TArray<UBillboardComponent*>& UBillboardSubsystem::GetRegisteredComponents() const {
    return mComponents;
}

void UBillboardSubsystem::OnDeinitialize() {
    mComponents.clear();
}
