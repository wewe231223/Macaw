#pragma once
#include "Core/CoreMinimal.h"
#include "ImGui/imgui.h"
#include "Editor/Panel/FEditorWindow.h"
#include "Editor/Message/FEditorInfo.h"
#include "Editor/Panel/FPropertyEditorContext.h"
#include "Core/Channel/FStateChannel.h"
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
    FPropertyPanel(FWorldEditorContext& InEditorContext, FStateChannel<Uint8>::FReadWriter InGizmoMode, FStateChannel<Uint8>::FReadWriter InGizmoCoordinateSpace, FAssetThumbnailRenderer* InThumbnailRenderer);

private:
    void DrawContents() override;

    static FString GetComponentTypeName(const UActorComponent& Component);

    void DrawGizmoControls();

    void DrawComponentList(AActor& Actor);

    void DrawSceneComponentTree(AActor& Actor, USceneComponent& Component);

    void DrawComponentNode(UActorComponent& Component, ImGuiTreeNodeFlags Flags);

    template <typename T>
    void AddComponent(AActor& Actor);

    void HandleDeleteShortcut(AActor& Actor, UActorComponent& Component);

private:
    FWorldEditorContext* mEditorContext{nullptr};
    FPropertyEditorContext mPropertyEditor{};
    FStateChannel<Uint8>::FReadWriter mGizmoMode{};
    FStateChannel<Uint8>::FReadWriter mGizmoCoordinateSpace{};
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

        if (Selected != nullptr && Selected->GetOwner() == &Actor && (Selected->GetTypeInfo()->IsA(UStaticMeshComponent::StaticTypeInfo()) || Selected->GetTypeInfo()->IsA(UBillboardComponent::StaticTypeInfo()))) {
            NewComponent->SetTargetComponent(static_cast<UPrimitiveComponent*>(Selected));
        }
    }

    mEditorContext->SetSelectedComponent(NewComponent);
}
