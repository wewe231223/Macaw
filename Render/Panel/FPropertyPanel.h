#pragma once

#include "PCH.h"
#include "ImGui/imgui.h"
#include "FEditorWindow.h"
#include "FEditorInfo.h"
#include "FPropertyEditorContext.h"
#include "Core/Channel/FStateChannel.h"
#include "Scene/AActor.h"
#include "Scene/FWorldEditorContext.h"
#include "Scene/Component/UActorComponent.h"
#include "Scene/Component/UBoxColliderComponent.h"
#include "Scene/Component/UCameraComponent.h"
#include "Scene/Component/UDirectionalLightComponent.h"
#include "Scene/Component/UPointLightComponent.h"
#include "Scene/Component/USceneComponent.h"
#include "Scene/Component/USpotLightComponent.h"
#include "Scene/Component/UStaticMeshComponent.h"
#include "Scene/Component/UBillboardTextComponent.h"
#include "Scene/Component/UNameTagComponent.h"
#include "../EditorView/FAssetThumbnailRenderer.h"

// 목록, 선택, 구조 변경만 담당합니다. 타입별 Details는 Component::DrawPanels()로 위임합니다.
class FPropertyPanel : public FEditorWindow {
public:
    FPropertyPanel(FWorldEditorContext& InEditorContext, FStateChannel<Uint8>::FReadWriter InGizmoMode, FStateChannel<Uint8>::FReadWriter InGizmoCoordinateSpace, FAssetThumbnailRenderer* InThumbnailRenderer);

private:
    void DrawContents() override;

    static const char* GetComponentTypeName(const UActorComponent& Component);

    void DrawGizmoControls();

    void DrawComponentList(AActor& Actor);

    void DrawSceneComponentTree(AActor& Actor, USceneComponent& Component);

    void DrawComponentNode(UActorComponent& Component, ImGuiTreeNodeFlags Flags);

    template <typename T> void AddSceneComponent(AActor& Actor);

    void HandleDeleteShortcut(AActor& Actor, UActorComponent& Component);

private:
    FWorldEditorContext* mEditorContext{nullptr};
    FPropertyEditorContext mPropertyEditor{};
    FStateChannel<Uint8>::FReadWriter mGizmoMode{};
    FStateChannel<Uint8>::FReadWriter mGizmoCoordinateSpace{};
};

template <typename T> void FPropertyPanel::AddSceneComponent(AActor& Actor) {
    static_assert(std::is_base_of_v<USceneComponent, T>);
    T* NewComponent{Actor.AddComponent<T>()};
    if (USceneComponent * Root{Actor.GetRootComponent()})
        NewComponent->AttachToComponent(Root);
    else
        Actor.SetRootComponent(NewComponent);
    mEditorContext->SetSelectedComponent(NewComponent);
}
