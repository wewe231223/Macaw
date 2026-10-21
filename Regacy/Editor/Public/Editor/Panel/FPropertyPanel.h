#pragma once
#include "CoreUObject/Reflection/UClass.h"
#include "Core/CoreMinimal.h"
#include "ImGui/imgui.h"
#include "Editor/Panel/FEditorWindow.h"
#include "Editor/Message/FEditorInfo.h"
#include "Editor/Panel/FPropertyEditorContext.h"
#include "World/AActor.h"
#include "Editor/World/FWorldEditorContext.h"
#include "World/Component/UActorComponent.h"
#include "World/Component/UBoxColliderComponent.h"
#include "World/Component/UCameraComponent.h"
#include "World/Component/UDirectionalLightComponent.h"
#include "World/Component/UPointLightComponent.h"
#include "World/Component/USceneComponent.h"
#include "World/Component/USpotLightComponent.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/Component/UBillboardTextComponent.h"
#include "World/Component/UBillboardComponent.h"
#include "World/Component/UFlipBookComponent.h"
#include "World/Component/UUVScrollComponent.h"
#include "Editor/View/FAssetThumbnailRenderer.h"

class FPropertyPanel : public FEditorWindow {
public:
    FPropertyPanel(FWorldEditorContext& InEditorContext, FAssetThumbnailRenderer* InThumbnailRenderer);

private:
    void DrawContents() override;

    static FString GetComponentTypeName(const UActorComponent& Component);

    void DrawComponentList(AActor& Actor);

    void DrawSceneComponentTree(AActor& Actor, USceneComponent& Component);

    void DrawComponentNode(UActorComponent& Component, ImGuiTreeNodeFlags Flags);

    void DrawComponentDragSource(AActor& Actor, USceneComponent& Component);
    void AcceptComponentDrop(AActor& Actor, USceneComponent* Parent);
    void DrawRootComponentDropTarget(AActor& Actor);
    bool CanAttachComponent(const AActor& Actor, const USceneComponent& Component, const USceneComponent* Parent) const;
    void ApplyPendingComponentDrop(AActor& Actor);

    template <typename T>
    void AddComponent(AActor& Actor);

    void HandleDeleteShortcut(AActor& Actor, UActorComponent& Component);

private:
    FWorldEditorContext* mEditorContext{nullptr};
    FPropertyEditorContext mPropertyEditor{};
    TObjectRef<USceneComponent> mPendingDraggedComponent{};
    TObjectRef<USceneComponent> mPendingParentComponent{};
};

template <typename T>
void FPropertyPanel::AddComponent(AActor& Actor) {
    static_assert(std::is_base_of_v<UActorComponent, T>);

    T* NewComponent{Actor.AddComponent<T>()};

    if (NewComponent == nullptr) {
        return;
    }

    if constexpr (std::is_base_of_v<USceneComponent, T>) {
        if (USceneComponent* Root{Actor.GetRootComponent()}) {
            NewComponent->AttachToComponent(Root);
        } else {
            Actor.SetRootComponent(NewComponent);
        }
    } else if constexpr (std::is_base_of_v<USubUVComponent, T>) {
        UActorComponent* Selected{mEditorContext->GetSelectedComponent()};

        if (Selected != nullptr && Selected->GetOwner() == &Actor && (Selected->GetClass()->IsChildOf(UStaticMeshComponent::StaticClass()) || Selected->GetClass()->IsChildOf(UBillboardComponent::StaticClass()))) {
            NewComponent->SetTargetComponent(static_cast<UPrimitiveComponent*>(Selected));
        }
    }

    mEditorContext->SetSelectedComponent(NewComponent);
}
