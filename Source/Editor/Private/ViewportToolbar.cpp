/**
 * @file ViewportToolbar.cpp
 * @brief 공통 스타일과 뷰포트 클라이언트 명령으로 변환 도구 툴바를 구성합니다.
 * @ingroup Editor
 */

#include "ViewportToolbar.h"
#include "EditorStyle.h"
#include "Editor/FEditorViewportClient.h"

#include <array>

/**
 * @brief 툴바와 메뉴에서 공유하는 변환 명령 목록과 표시 보조 함수를 제공합니다.
 */
namespace {
    /**
     * @brief 이동, 회전, 스케일 순서로 표시할 공통 명령 목록입니다.
     */
    const std::array<EViewportCommand, 3> TransformCommands{EViewportCommand::Translate, EViewportCommand::Rotate, EViewportCommand::Scale};
    /**
     * @brief TransformCommands와 같은 인덱스에 대응하는 버튼 및 메뉴 항목 이름입니다.
     */
    const std::array<const char*, 3> TransformLabels{"Translate", "Rotate", "Scale"};
    /**
     * @brief TransformCommands와 같은 인덱스에 대응하는 공통 스타일 아이콘입니다.
     */
    const std::array<EditorStyle::EIcon, 3> TransformIcons{EditorStyle::EIcon::Translate, EditorStyle::EIcon::Rotate, EditorStyle::EIcon::Scale};

    /**
     * @brief 동일한 변환 명령 목록을 아이콘 버튼 또는 메뉴 항목으로 표시합니다.
     * @param[in,out] Client 명령 실행과 상태 조회에 사용할 뷰포트 클라이언트입니다.
     * @param[in] Menu true이면 메뉴 항목으로 표시하며 false이면 한 줄의 버튼으로 표시합니다.
     */
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

    /**
     * @brief 월드와 로컬 좌표계 명령의 실행 가능 여부와 선택 상태를 메뉴에 표시합니다.
     * @param[in,out] Client 좌표계 조회와 변경 명령을 처리할 뷰포트 클라이언트입니다.
     */
    void DrawCoordinateMenu(FEditorViewportClient* Client) {
        if (EditorStyle::IconMenuItem("World", EditorStyle::EIcon::World, Client->IsCommandChecked(EViewportCommand::WorldSpace), Client->CanExecuteCommand(EViewportCommand::WorldSpace))) {
            Client->ExecuteCommand(EViewportCommand::WorldSpace);
        }

        if (EditorStyle::IconMenuItem("Local", EditorStyle::EIcon::Local, Client->IsCommandChecked(EViewportCommand::LocalSpace), Client->CanExecuteCommand(EViewportCommand::LocalSpace))) {
            Client->ExecuteCommand(EViewportCommand::LocalSpace);
        }
    }

    /**
     * @brief 그리드 표시와 간격 설정을 위한 메뉴 항목을 표시합니다.
     * @param[in,out] Client 그리드 명령 지원 여부를 조회하고 선택 요청을 전달할 뷰포트 클라이언트입니다.
     * @note 현재 그리드 명령을 지원하지 않으므로 실제 표시와 간격 조절 항목은 비활성입니다.
     */
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

/**
 * @brief 공통 스타일로 툴바와 드롭다운을 구성하고 입력을 클라이언트 명령으로 전달합니다.
 */
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
        if (ImGui::MenuItem("Select", "Q", Client->IsCommandChecked(EViewportCommand::Select), Client->CanExecuteCommand(EViewportCommand::Select))) {
            Client->ExecuteCommand(EViewportCommand::Select);
        }
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

    ImGui::SameLine(0.0f, 6.0f);
    if (ImGui::Button("Snap")) {
        ImGui::OpenPopup("GizmoSnapping");
    }
    if (ImGui::BeginPopup("GizmoSnapping")) {
        ImGui::BeginDisabled(!Client->CanExecuteCommand(EViewportCommand::Translate));
        auto* Settings{Client->GetGizmo().GetSettings()};
        ImGui::Checkbox("Translation Snap", &Settings->mTranslationSnap);
        ImGui::InputFloat("Translation Step", &Settings->mTranslationStep);
        ImGui::Checkbox("Rotation Snap", &Settings->mRotationSnap);
        ImGui::InputFloat("Rotation Degrees", &Settings->mRotationStep);
        ImGui::Checkbox("Scale Snap", &Settings->mScaleSnap);
        ImGui::InputFloat("Scale Step", &Settings->mScaleStep);
        ImGui::Checkbox("Surface Snap", &Settings->mSurfaceSnap);
        ImGui::Checkbox("Align to Surface", &Settings->mAlignToSurface);
        ImGui::InputFloat("Surface Offset", &Settings->mSurfaceOffset);
        ImGui::SliderFloat("Gizmo Size", &Settings->mSize, 0.5f, 2.0f);
        ImGui::EndDisabled();
        ImGui::Separator();
        ImGui::TextDisabled("Q/W/E/R: Mode | Tab: World/Local | X: Snap | Z: Surface");
        ImGui::TextDisabled("V: Vertex | MMB: Pivot | Alt+Drag: Duplicate");
        ImGui::EndPopup();
    }
    ImGui::EndGroup();
    EditorStyle::PopViewportToolbar();
    ImGui::PopID();
}
