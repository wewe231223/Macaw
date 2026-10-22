#include "Editor/Panel/FOutlinerPanel.h"
#include "Editor/EditorEngine.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/AActor.h"
#include "Engine/EPrimitiveType.h"

#include <imgui.h>
#include <format>

FOutlinerPanel::FOutlinerPanel(UEditorEngine& Engine)
	: FEditorPanel{Engine, "World Outliner"} {
}

FOutlinerPanel::~FOutlinerPanel() = default;

void FOutlinerPanel::DrawContent(float) {
    UEditorEngine& Engine{GetEditorEngine()};

    if (ImGui::Button("+ Plane")) {
        Engine.SpawnPrimitive(EPrimitiveType::Plane);
    }

    ImGui::SameLine();

    if (ImGui::Button("+ Cube")) {
        Engine.SpawnPrimitive(EPrimitiveType::Cube);
    }

    ImGui::SameLine();

    if (ImGui::Button("+ Sphere")) {
        Engine.SpawnPrimitive(EPrimitiveType::Sphere);
    }

    ImGui::Separator();

    const UWorld* World{Engine.GetWorld()};

    if (World != nullptr && World->GetPersistentLevel() != nullptr) {
        ImGui::TextUnformatted(std::format("PersistentLevel ({})", World->GetPersistentLevel()->GetActors().size()).c_str());

        for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
            ImGui::PushID(Actor.get());

            if (ImGui::Selectable(Actor->GetName().c_str(), Engine.GetSelectedActor() == Actor.get())) {
                Engine.SelectActor(Actor->GetHandle());
            }

            ImGui::PopID();
        }
    }

    ImGui::Separator();

    ImGui::BeginDisabled(Engine.GetSelectedActor() == nullptr);

    if (ImGui::Button("Delete Selected")) {
        Engine.DestroySelectedActor();
    }

    ImGui::EndDisabled();
}
