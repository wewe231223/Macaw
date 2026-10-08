#include "pch.h"
#include "World/Subsystem/UOverlaySubsystem.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/UWorld.h"

void UOverlaySubsystem::BuildDrawData(FOverlayRenderData& Overlay, FObjectHandle SelectedActor) const {
    Overlay.mSelectionMeshes.clear();

    if (GetWorld() == nullptr || !SelectedActor.IsValid()) {
        return;
    }

    for (const UActorComponent* Registered : GetWorld()->GetRenderSubsystem().GetRegisteredComponents()) {
        const UStaticMeshComponent* Component{Registered != nullptr && Registered->GetTypeInfo()->IsA(UStaticMeshComponent::StaticTypeInfo()) ? static_cast<const UStaticMeshComponent*>(Registered) : nullptr};

        if (Component == nullptr || !Component->IsRegistered() || !Component->IsVisible() || Component->GetOwner() == nullptr || Component->GetOwner()->GetHandle() != SelectedActor) {
            continue;
        }

        const std::unique_ptr<FPrimitiveSceneProxy> Proxy{Component->CreateSceneProxy()};
        const FMeshSceneData* Mesh{Proxy != nullptr ? Proxy->GetMeshData() : nullptr};

        if (Mesh != nullptr) {
            const FPrimitiveTransform& Transform{Proxy->GetTransform()};

            Overlay.mSelectionMeshes.push_back(FOverlayMeshDrawData{Transform.mWorld, *Mesh, Transform.mWorldSphereBounds});
        }
    }
}
