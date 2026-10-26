/**
 * @file FOutlinerPanel.cpp
 * @brief 월드의 액터와 컴포넌트 계층 및 선택 상태를 표시합니다.
 * @ingroup Editor
 */

#include "Editor/Panel/FOutlinerPanel.h"
#include "Editor/FEditorContext.h"
#include "Engine/AActor.h"
#include "Engine/UWorld.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/Component/UActorComponent.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/EPrimitiveType.h"

#include <imgui.h>
#include <format>
#include <algorithm>

/**
 * @brief FOutlinerPanel의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FOutlinerPanel::FOutlinerPanel(FEditorContext* Context)
	: FEditorPanel{Context, "World Outliner"} {
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FOutlinerPanel::~FOutlinerPanel() = default;

/**
 * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
 */
void FOutlinerPanel::DrawContent(float) {
    FEditorContext* Context{GetEditorContext()};
    ImGui::BeginDisabled(!Context->CanEditWorld());

    if (ImGui::Button("+ 액터")) {
        ImGui::OpenPopup("AddActor");
    }
    if (ImGui::BeginPopup("AddActor")) {
        if (ImGui::MenuItem("Plane")) {
            Context->SpawnPrimitive(EPrimitiveType::Plane);
        }
        if (ImGui::MenuItem("Cube")) {
            Context->SpawnPrimitive(EPrimitiveType::Cube);
        }
        if (ImGui::MenuItem("Sphere")) {
            Context->SpawnPrimitive(EPrimitiveType::Sphere);
        }
        if (ImGui::MenuItem("Camera")) {
            Context->SpawnCamera();
        }
        if (ImGui::MenuItem("Text")) {
            Context->SpawnText();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(Context->GetSelectedCamera() == nullptr);
    if (ImGui::Button("Use as PIE Camera")) {
        Context->UseSelectedCamera();
    }
    ImGui::EndDisabled();

    ImGui::EndDisabled();
    const UCameraComponent* Camera{Context->GetWorld() != nullptr ? Context->GetWorld()->GetActiveCamera() : nullptr};
    ImGui::TextDisabled("PIE Camera: %s", Camera != nullptr ? Camera->GetOwner()->GetName().c_str() : "None");
    ImGui::Separator();
    const auto Actors{Context->GetActorHandles()};
    ImGui::TextUnformatted(std::format("PersistentLevel ({})", Actors.size()).c_str());

    for (FObjectHandle Handle : Actors) {
        AActor* Actor{Cast<AActor>(Context->ResolveObject(Handle))};

        if (Actor == nullptr) {
            continue;
        }

        TArray<FObjectHandle> Components{};

        for (const auto& Component : Actor->GetComponents()) {
            Components.push_back(Component->GetHandle());
        }

        ImGui::PushID(Actor);
        const TArray<FObjectHandle> Selection{Context->GetSelectedActors()};
        const bool Selected{std::find(Selection.begin(), Selection.end(), Handle) != Selection.end()};
        const ImGuiTreeNodeFlags Flags{ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth | (Selected ? ImGuiTreeNodeFlags_Selected : 0) | (Components.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : 0)};
        const bool Open{ImGui::TreeNodeEx(Actor->GetName().c_str(), Flags)};

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            if (ImGui::GetIO().KeyCtrl) {
                Context->ToggleActorSelection(Handle);
            } else {
                Context->SelectActor(Handle);
            }
        }

        if (Open && !Components.empty()) {
            for (FObjectHandle ComponentHandle : Components) {
                if (UObject* Component{Context->ResolveObject(ComponentHandle)}) {
                    ImGui::PushID(Component);

                    if (ImGui::Selectable(Component->GetName().c_str(), Context->GetSelectedObject() == Component)) {
                        Context->SelectObject(ComponentHandle);
                    }

                    ImGui::PopID();
                }
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::BeginDisabled(!Context->CanEditWorld() || Context->GetSelectedActor() == nullptr);

    if (ImGui::Button("Delete Selected")) {
        Context->DestroySelectedActor();
    }

    ImGui::EndDisabled();
}
