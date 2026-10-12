#include "pch.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Editor/Property/FComponentDetails.h"
#include "Editor/Panel/FPropertyPanel.h"
#include "World/UWorld.h"

#include <algorithm>
#include <cstring>

namespace {
    constexpr const char* ComponentDragDropPayloadType{"PROPERTY_SCENE_COMPONENT"};
}

FPropertyPanel::FPropertyPanel(FWorldEditorContext& InEditorContext, FStateChannel<Uint8>::FReadWriter InGizmoMode, FStateChannel<Uint8>::FReadWriter InGizmoCoordinateSpace, FAssetThumbnailRenderer* InThumbnailRenderer)
	: FEditorWindow("Property Window"),
	  mEditorContext(&InEditorContext),
	  mGizmoMode(std::move(InGizmoMode)),
	  mGizmoCoordinateSpace(std::move(InGizmoCoordinateSpace)) {
    mPropertyEditor.BindThumbnailRenderer(InThumbnailRenderer);
}

void FPropertyPanel::DrawContents() {
    if (mEditorContext == nullptr)
        return;

    AActor* Actor{mEditorContext->GetSelectedActor()};

    if (Actor == nullptr) {
        ImGui::TextDisabled("Select an actor to inspect its components.");
        return;
    }

    DrawGizmoControls();
    DrawComponentList(*Actor);
    ImGui::Separator();

    if (UActorComponent* Component{mEditorContext->GetSelectedComponent()}; Component != nullptr && Component->GetOwner() == Actor) {
        ImGui::Text("Details: %s", GetComponentTypeName(*Component).c_str());
        ImGui::PushID(Component);

        UWorld* World{Actor->GetWorld()};

        mPropertyEditor.BindAssetRegistry(World != nullptr ? World->GetAssetRegistry() : nullptr);
        FComponentDetails::Draw(*Component, mPropertyEditor);
        ImGui::Separator();
        HandleDeleteShortcut(*Actor, *Component);
        ImGui::PopID();
    } else {
        ImGui::TextDisabled("Select a component.");
    }
}

FString FPropertyPanel::GetComponentTypeName(const UActorComponent& Component) {
    return Component.GetClass()->GetClassName().ToString();
}

void FPropertyPanel::DrawGizmoControls() {
    ImGui::TextUnformatted("Gizmo Mode");

    int ModeIndex{static_cast<int>(mGizmoMode.Read())};
    bool BModeChanged{false};

    BModeChanged |= ImGui::RadioButton("Translate", &ModeIndex, static_cast<int>(EGizmoMode::Translate));
    ImGui::SameLine();
    BModeChanged |= ImGui::RadioButton("Rotate", &ModeIndex, static_cast<int>(EGizmoMode::Rotate));
    ImGui::SameLine();
    BModeChanged |= ImGui::RadioButton("Scale", &ModeIndex, static_cast<int>(EGizmoMode::Scale));

    if (BModeChanged)
        mGizmoMode.Emplace(static_cast<Uint8>(ModeIndex));

    ImGui::TextUnformatted("Coordinate Mode");

    int CoordinateSpaceIndex{static_cast<int>(mGizmoCoordinateSpace.Read())};
    bool BCoordinateSpaceChanged{false};

    BCoordinateSpaceChanged |= ImGui::RadioButton("World", &CoordinateSpaceIndex, static_cast<int>(EGizmoCoordinateSpace::World));
    ImGui::SameLine();
    BCoordinateSpaceChanged |= ImGui::RadioButton("Local", &CoordinateSpaceIndex, static_cast<int>(EGizmoCoordinateSpace::Local));

    if (BCoordinateSpaceChanged)
        mGizmoCoordinateSpace.Emplace(static_cast<Uint8>(CoordinateSpaceIndex));

    ImGui::Separator();
}

void FPropertyPanel::DrawComponentList(AActor& Actor) {
    ImGui::TextUnformatted("Components");
    ImGui::SameLine();

    if (ImGui::Button("+ Add Component"))
        ImGui::OpenPopup("AddComponentPopup");

    if (ImGui::BeginPopup("AddComponentPopup")) {
        ImGui::TextDisabled("Add to selected actor");
        ImGui::Separator();

        if (ImGui::MenuItem("Scene Component"))
            AddComponent<USceneComponent>(Actor);

        if (ImGui::MenuItem("Static Mesh Component"))
            AddComponent<UStaticMeshComponent>(Actor);

        if (ImGui::MenuItem("Camera Component"))
            AddComponent<UCameraComponent>(Actor);

        if (ImGui::MenuItem("Directional Light Component"))
            AddComponent<UDirectionalLightComponent>(Actor);

        if (ImGui::MenuItem("Point Light Component"))
            AddComponent<UPointLightComponent>(Actor);

        if (ImGui::MenuItem("Spot Light Component"))
            AddComponent<USpotLightComponent>(Actor);

        if (ImGui::MenuItem("Box Collider Component"))
            AddComponent<UBoxColliderComponent>(Actor);

        if (ImGui::MenuItem("Billboard Text Component"))
            AddComponent<UBillboardTextComponent>(Actor);

        if (ImGui::MenuItem("Billboard Component")) {
            AddComponent<UBillboardComponent>(Actor);
        }

        if (ImGui::MenuItem("SubUV Component")) {
            AddComponent<USubUVComponent>(Actor);
        }

        if (ImGui::MenuItem("FlipBook Component")) {
            AddComponent<UFlipBookComponent>(Actor);
        }

        if (ImGui::MenuItem("UVScroll Component")) {
            AddComponent<UUVScrollComponent>(Actor);
        }

        ImGui::EndPopup();
    }

    ImGui::BeginChild("ComponentList", ImVec2(0.0f, 180.0f), true);

    for (const std::unique_ptr<UActorComponent>& Component : Actor.GetComponents()) {
        USceneComponent* SceneComponent{Component->GetClass()->IsChildOf(USceneComponent::StaticClass()) ? static_cast<USceneComponent*>(Component.get()) : nullptr};

        if (SceneComponent == nullptr) {
            DrawComponentNode(*Component, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        } else if (SceneComponent->GetParent() == nullptr || SceneComponent->GetParent()->GetOwner() != &Actor) {
            DrawSceneComponentTree(Actor, *SceneComponent);
        }
    }

    DrawRootComponentDropTarget(Actor);
    ImGui::EndChild();
    ApplyPendingComponentDrop(Actor);
}

void FPropertyPanel::DrawSceneComponentTree(AActor& Actor, USceneComponent& Component) {
    const bool BHasOwnedChildren{std::ranges::any_of(Component.GetChildren(), [&Actor](const TObjectRef<USceneComponent>& Child) {
        return Child.Get() != nullptr && Child.Get()->GetOwner() == &Actor;
    })};

    ImGuiTreeNodeFlags Flags{ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth};

    if (!BHasOwnedChildren)
        Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    if (mEditorContext->GetSelectedComponent() == &Component)
        Flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::PushID(&Component);

    const bool BOpen{ImGui::TreeNodeEx("Component", Flags, "%s", GetComponentTypeName(Component).c_str())};

    if (ImGui::IsItemClicked())
        mEditorContext->SetSelectedComponent(&Component);

    DrawComponentDragSource(Actor, Component);
    AcceptComponentDrop(Actor, &Component);

    if (BHasOwnedChildren && BOpen) {
        for (const TObjectRef<USceneComponent>& Child : Component.GetChildren()) {
            if (USceneComponent* ChildComponent{Child.Get()}; ChildComponent != nullptr && ChildComponent->GetOwner() == &Actor) {
                DrawSceneComponentTree(Actor, *ChildComponent);
            }
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void FPropertyPanel::DrawComponentNode(UActorComponent& Component, ImGuiTreeNodeFlags Flags) {
    if (mEditorContext->GetSelectedComponent() == &Component)
        Flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::PushID(&Component);
    ImGui::TreeNodeEx("Component", Flags, "%s", GetComponentTypeName(Component).c_str());

    if (ImGui::IsItemClicked())
        mEditorContext->SetSelectedComponent(&Component);

    ImGui::PopID();
}

void FPropertyPanel::HandleDeleteShortcut(AActor& Actor, UActorComponent& Component) {
    const ImGuiIO& IO{ImGui::GetIO()};

    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || IO.WantTextInput || ImGui::IsAnyItemActive() || !ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        return;
    }

    Component.DestroyComponent();
    mEditorContext->SetSelectedActor(&Actor);
}

void FPropertyPanel::DrawComponentDragSource(AActor& Actor, USceneComponent& Component) {
    if (Actor.IsBeingDestroyed() || Component.IsBeingDestroyed() || Actor.GetRootComponent() == &Component || !ImGui::BeginDragDropSource()) {
        return;
    }

    const FObjectHandle ComponentHandle{Component.GetHandle()};

    ImGui::SetDragDropPayload(ComponentDragDropPayloadType, &ComponentHandle, sizeof(ComponentHandle));
    ImGui::TextUnformatted(Component.GetName().ToString().c_str());
    ImGui::EndDragDropSource();
}

void FPropertyPanel::AcceptComponentDrop(AActor& Actor, USceneComponent* Parent) {
    const ImGuiPayload* Payload{ImGui::GetDragDropPayload()};

    if (Payload == nullptr || !Payload->IsDataType(ComponentDragDropPayloadType) || Payload->DataSize != sizeof(FObjectHandle)) {
        return;
    }

    FObjectHandle ComponentHandle{};

    std::memcpy(&ComponentHandle, Payload->Data, sizeof(ComponentHandle));

    const TObjectRef<USceneComponent> ComponentRef{ComponentHandle};
    USceneComponent* Component{ComponentRef.Get()};

    if (Component == nullptr || !CanAttachComponent(Actor, *Component, Parent) || !ImGui::BeginDragDropTarget()) {
        return;
    }

    if (const ImGuiPayload* AcceptedPayload{ImGui::AcceptDragDropPayload(ComponentDragDropPayloadType)}; AcceptedPayload != nullptr && AcceptedPayload->Delivery) {
        mPendingDraggedComponent.SetHandle(ComponentHandle);
        mPendingParentComponent.Set(Parent);
    }

    ImGui::EndDragDropTarget();
}

void FPropertyPanel::DrawRootComponentDropTarget(AActor& Actor) {
    const float AvailableHeight{std::max(ImGui::GetFrameHeight(), ImGui::GetContentRegionAvail().y)};

    ImGui::Selectable("##RootComponentDropTarget", false, ImGuiSelectableFlags_None, ImVec2{0.0f, AvailableHeight});
    AcceptComponentDrop(Actor, nullptr);
}

bool FPropertyPanel::CanAttachComponent(const AActor& Actor, const USceneComponent& Component, const USceneComponent* Parent) const {
    if (Actor.IsBeingDestroyed() || Component.IsBeingDestroyed() || Component.GetOwner() != &Actor || Actor.GetRootComponent() == &Component || Component.GetParent() == Parent || (Parent != nullptr && (Parent->IsBeingDestroyed() || Parent->GetOwner() != &Actor))) {
        return false;
    }

    for (const USceneComponent* Ancestor{Parent}; Ancestor != nullptr; Ancestor = Ancestor->GetParent()) {
        if (Ancestor == &Component) {
            return false;
        }
    }

    if (Parent != nullptr) {
        FTransform RelativeTransform{};

        return Component.GetComponentTransform().MakeRelativeTo(Parent->GetComponentTransform(), RelativeTransform);
    }

    return true;
}

void FPropertyPanel::ApplyPendingComponentDrop(AActor& Actor) {
    USceneComponent* Component{mPendingDraggedComponent.Get()};
    USceneComponent* Parent{mPendingParentComponent.Get()};
    const bool HasParent{mPendingParentComponent.GetHandle().IsValid()};

    mPendingDraggedComponent.Reset();
    mPendingParentComponent.Reset();

    if (Component != nullptr && (!HasParent || Parent != nullptr) && CanAttachComponent(Actor, *Component, Parent) && Component->AttachToComponent(Parent, EAttachmentTransformRule::KeepWorldTransform)) {
        mEditorContext->SetSelectedComponent(Component);
    }
}
