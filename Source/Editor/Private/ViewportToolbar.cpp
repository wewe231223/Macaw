#include "ViewportToolbar.h"
#include "EditorStyle.h"
#include "Editor/ViewportClient.h"

#include <array>

namespace {
    const std::array<EViewportCommand, 3> TransformCommands{EViewportCommand::Translate, EViewportCommand::Rotate, EViewportCommand::Scale};
    const std::array<const char*, 3> TransformLabels{"Translate", "Rotate", "Scale"};
    const std::array<EditorStyle::EIcon, 3> TransformIcons{EditorStyle::EIcon::Translate, EditorStyle::EIcon::Rotate, EditorStyle::EIcon::Scale};

    void DrawModeCommands(FEditorViewportClient* Client, bool Menu) {
        for (std::size_t Index{}; Index < TransformCommands.size(); ++Index) {
            if (!Menu && Index > 0) {
                ImGui::SameLine();
            }

            const EViewportCommand Command{TransformCommands[Index]};
            const bool Selected{Client->IsCommandChecked(Command)};
            const bool Enabled{Client->CanExecuteCommand(Command)};
            const bool Pressed{Menu ? EditorStyle::IconMenuItem(TransformLabels[Index], TransformIcons[Index], Selected, Enabled) : EditorStyle::IconButton(TransformLabels[Index], TransformIcons[Index], Selected, Enabled)};

            if (Pressed) {
                Client->ExecuteCommand(Command);
            }
        }
    }

    void DrawCoordinateMenu(FEditorViewportClient* Client) {
        if (EditorStyle::IconMenuItem("World", EditorStyle::EIcon::World, Client->IsCommandChecked(EViewportCommand::WorldSpace), Client->CanExecuteCommand(EViewportCommand::WorldSpace))) {
            Client->ExecuteCommand(EViewportCommand::WorldSpace);
        }

        if (EditorStyle::IconMenuItem("Local", EditorStyle::EIcon::Local, Client->IsCommandChecked(EViewportCommand::LocalSpace), Client->CanExecuteCommand(EViewportCommand::LocalSpace))) {
            Client->ExecuteCommand(EViewportCommand::LocalSpace);
        }
    }

    void DrawGridSettings(FEditorViewportClient* Client) {
        ImGui::SeparatorText("Viewport Grid");

        if (ImGui::MenuItem("Show Grid", nullptr, Client->IsCommandChecked(EViewportCommand::ToggleGrid), Client->CanExecuteCommand(EViewportCommand::ToggleGrid))) {
            Client->ExecuteCommand(EViewportCommand::ToggleGrid);
        }

        ImGui::MenuItem("Grid Spacing...", nullptr, false, Client->CanExecuteCommand(EViewportCommand::ToggleGrid));
        ImGui::Separator();
        ImGui::TextDisabled("Viewport grid is not available yet.");
    }
}

void ViewportToolbar::Draw(FEditorViewportClient* Client) {
    ImGui::PushID("ViewportToolbar");
    EditorStyle::PushViewportToolbar();
    ImGui::BeginGroup();

    if (EditorStyle::IconButton("Transform Tools", EditorStyle::EIcon::Tools, ImGui::IsPopupOpen("TransformTools"), true, true)) {
        ImGui::OpenPopup("TransformTools");
    }

    ImGui::SetNextWindowPos(ImVec2{ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 4.0f}, ImGuiCond_Appearing);

    if (ImGui::BeginPopup("TransformTools")) {
        ImGui::SeparatorText("Transform Tools");
        DrawModeCommands(Client, true);
        ImGui::Separator();

        if (ImGui::BeginMenu("Coordinate System", Client->CanExecuteCommand(EViewportCommand::WorldSpace) || Client->CanExecuteCommand(EViewportCommand::LocalSpace))) {
            DrawCoordinateMenu(Client);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Grid Settings")) {
            DrawGridSettings(Client);
            ImGui::EndMenu();
        }

        ImGui::Separator();
        ImGui::TextDisabled("Space: Cycle transform mode");
        ImGui::EndPopup();
    }

    ImGui::SameLine(0.0f, 6.0f);
    DrawModeCommands(Client, false);
    ImGui::SameLine(0.0f, 6.0f);

    const bool WorldSpace{Client->IsCommandChecked(EViewportCommand::WorldSpace)};
    const char* CoordinateTooltip{Client->GetGizmoMode() == EGizmoMode::Scale ? "Local Space\nScaling uses local axes." : WorldSpace ? "World Space (click to switch)" : "Local Space (click to switch)"};

    if (EditorStyle::IconButton("Coordinate System", WorldSpace ? EditorStyle::EIcon::World : EditorStyle::EIcon::Local, false, Client->CanExecuteCommand(EViewportCommand::ToggleCoordinateSystem), false, CoordinateTooltip)) {
        Client->ExecuteCommand(EViewportCommand::ToggleCoordinateSystem);
    }

    ImGui::SameLine(0.0f, 6.0f);

    if (EditorStyle::IconButton("Grid Settings", EditorStyle::EIcon::Grid, ImGui::IsPopupOpen("GridSettings"), true, true)) {
        ImGui::OpenPopup("GridSettings");
    }

    ImGui::SetNextWindowPos(ImVec2{ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 4.0f}, ImGuiCond_Appearing);

    if (ImGui::BeginPopup("GridSettings")) {
        DrawGridSettings(Client);
        ImGui::EndPopup();
    }

    ImGui::EndGroup();
    EditorStyle::PopViewportToolbar();
    ImGui::PopID();
}
