/**
 * @file ViewportToolbar.cpp
 * @brief 공통 스타일과 뷰포트 클라이언트 명령으로 변환 도구 툴바를 구성합니다.
 * @ingroup Editor
 */

#include "ViewportToolbar.h"
#include "EditorStyle.h"
#include "Editor/FEditorViewportClient.h"
#include "Editor/FViewportCommandInfo.h"

namespace Editor {

    /**
     * @brief 툴바와 메뉴에서 공유하는 변환 명령 목록과 표시 보조 함수를 제공합니다.
     */
    namespace {
        /**
         * @brief 동일한 변환 명령 목록을 아이콘 버튼 또는 메뉴 항목으로 표시합니다.
         * @param[in,out] Client 명령 실행과 상태 조회에 사용할 뷰포트 클라이언트입니다.
         * @param[in] Menu true이면 메뉴 항목으로 표시하며 false이면 한 줄의 버튼으로 표시합니다.
         */
        void DrawModeCommands(Editor::FEditorViewportClient* Client, bool Menu) {
            bool Drawn{};

            for (const Editor::FViewportCommandInfo& Info : Editor::FViewportCommandInfo::GetCommands()) {
                if (!Info.mTransformMode || (!Menu && Info.mCommand == Editor::EViewportCommand::Select)) {
                    continue;
                }

                if (!Menu && Drawn) {
                    ImGui::SameLine();
                }

                const Editor::EViewportCommand Command{Info.mCommand};
                const bool Selected{Client->IsCommandChecked(Command)};
                const bool Enabled{Client->CanExecuteCommand(Command)};
                const Core::FString Shortcut{Info.GetShortcutLabel()};
                const Core::FString Tooltip{Core::FString{Info.mLabel} + " (" + Shortcut + ")"};
                const bool Pressed{Menu ? Command == Editor::EViewportCommand::Select ? ImGui::MenuItem(Info.mLabel, Shortcut.c_str(), Selected, Enabled) : Editor::EditorStyle::IconMenuItem(Info.mLabel, Info.mIcon, Selected, Enabled) : Editor::EditorStyle::IconButton(Info.mLabel, Info.mIcon, Selected, Enabled, false, Tooltip.c_str())};

                Drawn = true;

                if (Pressed) {
                    Client->ExecuteCommand(Command);
                }
            }
        }

        /**
         * @brief 월드와 로컬 좌표계 명령의 실행 가능 여부와 선택 상태를 메뉴에 표시합니다.
         * @param[in,out] Client 좌표계 조회와 변경 명령을 처리할 뷰포트 클라이언트입니다.
         */
        void DrawCoordinateMenu(Editor::FEditorViewportClient* Client) {
            for (Editor::EViewportCommand Command : {Editor::EViewportCommand::WorldSpace, Editor::EViewportCommand::LocalSpace}) {
                const Editor::FViewportCommandInfo* Info{Editor::FViewportCommandInfo::Find(Command)};

                if (Editor::EditorStyle::IconMenuItem(Info->mLabel, Info->mIcon, Client->IsCommandChecked(Command), Client->CanExecuteCommand(Command))) {
                    Client->ExecuteCommand(Command);
                }
            }
        }

        /**
         * @brief 그리드 표시와 간격 설정을 위한 메뉴 항목을 표시합니다.
         * @param[in,out] Client 그리드 명령 지원 여부를 조회하고 선택 요청을 전달할 뷰포트 클라이언트입니다.
         */
        void DrawGridSettings(Editor::FEditorViewportClient* Client) {
            ImGui::SeparatorText("Viewport Grid");

            const Editor::FViewportCommandInfo* Info{Editor::FViewportCommandInfo::Find(Editor::EViewportCommand::ToggleGrid)};

            if (ImGui::MenuItem(Info->mLabel, nullptr, Client->IsCommandChecked(Editor::EViewportCommand::ToggleGrid), Client->CanExecuteCommand(Editor::EViewportCommand::ToggleGrid))) {
                Client->ExecuteCommand(Editor::EViewportCommand::ToggleGrid);
            }

            float Spacing{Client->GetGridSpacing()};

            if (ImGui::InputFloat("Grid Spacing", &Spacing)) {
                Client->SetGridSpacing(Spacing);
            }
        }

        /**
         * @brief 스냅 명령의 체크 상태를 표시하고 변경 요청을 공통 명령 경로로 전달합니다.
         * @param[in,out] Client 상태 조회와 명령 실행에 사용할 클라이언트입니다.
         * @param[in] Command 체크 상자에 연결할 스냅 전환 명령입니다.
         * @pre Client는 유효해야 하며 Command는 공통 명령 정보에 등록되어 있어야 합니다.
         */
        void DrawSettingToggle(Editor::FEditorViewportClient* Client, Editor::EViewportCommand Command) {
            const Editor::FViewportCommandInfo* Info{Editor::FViewportCommandInfo::Find(Command)};
            bool Checked{Client->IsCommandChecked(Command)};

            if (ImGui::Checkbox(Info->mLabel, &Checked)) {
                Client->ExecuteCommand(Command);
            }
        }

        /**
         * @brief 기즈모 수치 설정의 입력을 표시하고 변경 값을 클라이언트의 검증 경로로 전달합니다.
         * @param[in,out] Client 변경 값을 검증하고 적용할 유효한 클라이언트입니다.
         * @param[in] Setting 변경할 수치 항목입니다.
         * @param[in] Label 현재 UI 범위에서 입력을 식별할 표시 이름입니다.
         * @param[in] Value 현재 설정 값이며 입력 중에는 지역 사본만 수정합니다.
         * @note 크기는 슬라이더로, 나머지는 수치 입력으로 표시하며 기즈모 설정의 주소를 UI에 직접 전달하지 않습니다.
         */
        void DrawNumericSetting(Editor::FEditorViewportClient* Client, Editor::EGizmoSetting Setting, const char* Label, float Value) {
            const bool Changed{Setting == Editor::EGizmoSetting::Size ? ImGui::SliderFloat(Label, &Value, 0.5f, 2.0f) : ImGui::InputFloat(Label, &Value)};

            if (Changed) {
                Client->SetGizmoSetting(Setting, Value);
            }
        }

        /**
         * @brief View Mode, View Filter 체크박스와 카메라 감도 드롭다운을 표시합니다.
         * @param[in] Client 설정을 읽고 변경할 뷰포트 클라이언트입니다.
         */
        void DrawViewSettings(Editor::FEditorViewportClient* Client) {
            const float Scale{ImGui::GetStyle().FontScaleMain * ImGui::GetStyle().FontScaleDpi};
            const std::array<Engine::EViewMode, 5> Modes{Engine::EViewMode::Lit_Gouraud, Engine::EViewMode::Lit_Lambert, Engine::EViewMode::Lit_Phong, Engine::EViewMode::Unlit, Engine::EViewMode::Wireframe};
            const std::array<const char*, 5> Names{"Lit_Gouraud", "Lit_Lambert", "Lit_Phong", "Unlit", "Wireframe"};
            const auto Current{std::find(Modes.begin(), Modes.end(), Client->GetColorViewMode())};
            const std::size_t Index{Current != Modes.end() ? static_cast<std::size_t>(Current - Modes.begin()) : 2};
            const std::array<Engine::EViewMode, 3> Buffers{Client->GetColorViewMode(), Engine::EViewMode::WorldNormal, Engine::EViewMode::SceneDepth};
            const std::array<const char*, 3> BufferNames{"Scene Color", "World Normal", "Scene Depth"};
            const std::size_t BufferIndex{Client->GetViewMode() == Engine::EViewMode::WorldNormal ? 1u : Client->GetViewMode() == Engine::EViewMode::SceneDepth ? 2u : 0u};
            const char* Preview{BufferIndex == 0 ? Names[Index] : BufferNames[BufferIndex]};

            ImGui::SetNextItemWidth(175.0f * Scale);

            if (ImGui::BeginCombo("##ViewMode", Preview)) {
                for (std::size_t Mode{}; Mode < Modes.size(); ++Mode) {
                    if (ImGui::Selectable(Names[Mode], Client->GetViewMode() == Modes[Mode])) {
                        Client->SetViewMode(Modes[Mode]);
                    }
                }

                ImGui::Separator();

                if (ImGui::BeginMenu("Buffer Visualization")) {
                    for (std::size_t Buffer{}; Buffer < Buffers.size(); ++Buffer) {
                        if (ImGui::MenuItem(BufferNames[Buffer], nullptr, Buffer == BufferIndex)) {
                            Client->SetViewMode(Buffers[Buffer]);
                        }
                    }

                    ImGui::EndMenu();
                }

                ImGui::EndCombo();
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(115.0f * Scale);

            if (ImGui::BeginCombo("##ViewFilter", "View Filter")) {
                ImGui::SeparatorText("Show Flags");

                Engine::FEngineShowFlags Flags{Client->GetShowFlags()};
                bool Changed{ImGui::Checkbox("Primitives", &Flags.mPrimitives)};
                Changed = ImGui::Checkbox("Sprites / Billboards", &Flags.mSprites) || Changed;
                Changed = ImGui::Checkbox("Text", &Flags.mText) || Changed;
                Changed = ImGui::Checkbox("Light Icons", &Flags.mLightIcons) || Changed;
                Changed = ImGui::Checkbox("Shadows", &Flags.mShadows) || Changed;
                Changed = ImGui::Checkbox("Fog", &Flags.mFog) || Changed;
                Changed = ImGui::Checkbox("FXAA", &Flags.mFXAA) || Changed;
                Changed = ImGui::Checkbox("Fog Icons", &Flags.mFogIcons) || Changed;
                Changed = ImGui::Checkbox("Light Visualizers", &Flags.mLightVisualizers) || Changed;
                Changed = ImGui::Checkbox("Bounding Boxes (AABB)", &Flags.mBounds) || Changed;
                Changed = ImGui::Checkbox("Grid", &Flags.mGrid) || Changed;
                Changed = ImGui::Checkbox("World Axes", &Flags.mAxes) || Changed;
                ImGui::Separator();

                float Thickness{Client->GetLineThickness()};

                if (ImGui::SliderFloat("Line Width (px)", &Thickness, 0.5f, 10.0f, "%.1f")) {
                    Client->SetLineThickness(Thickness);
                }

                if (ImGui::MenuItem("Use Defaults")) {
                    Flags = {};
                    Changed = true;
                    Client->SetLineThickness(1.5f);
                }

                if (Changed) {
                    Client->SetShowFlags(Flags);
                }

                ImGui::EndCombo();
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(95.0f * Scale);

            if (ImGui::BeginCombo("##CameraSettings", "Camera")) {
                ImGui::SeparatorText("Camera Settings");

                float Sensitivity{Client->GetCameraSensitivity()};

                if (ImGui::SliderFloat("Sensitivity", &Sensitivity, 0.01f, 2.0f, "%.2f")) {
                    Client->SetCameraSensitivity(Sensitivity);
                }

                float Speed{Client->GetCameraSpeed()};

                if (ImGui::SliderFloat("Movement Speed", &Speed, 0.1f, 128.0f, "%.1f", ImGuiSliderFlags_Logarithmic)) {
                    Client->SetCameraSpeed(Speed);
                }

                Engine::FEditorCameraState Camera{Client->GetPerspectiveCamera()};
                float FieldOfView{Camera.GetFieldOfView()};
                float NearClip{Camera.GetNearClip()};
                float FarClip{Camera.GetFarClip()};
                bool ProjectionChanged{ImGui::SliderFloat("FOV", &FieldOfView, 1.0f, 179.0f, "%.1f")};
                ProjectionChanged = ImGui::InputFloat("Near Clip", &NearClip) || ProjectionChanged;
                ProjectionChanged = ImGui::InputFloat("Far Clip", &FarClip) || ProjectionChanged;

                if (ProjectionChanged && Camera.Set(Camera.GetLocation(), Camera.GetRotation(), FieldOfView, NearClip, FarClip)) {
                    Client->SetPerspectiveCamera(Camera);
                }

                ImGui::EndCombo();
            }
        }

    }

    /**
     * @fn void Editor::ViewportToolbar::Draw(Editor::FEditorViewportClient* Client)
     * @brief 공통 스타일로 툴바와 드롭다운을 구성하고 입력을 클라이언트 명령으로 전달합니다.
     */
    void Editor::ViewportToolbar::Draw(Editor::FEditorViewportClient* Client) {
        ImGui::PushID("ViewportToolbar");
        Editor::EditorStyle::PushViewportToolbar();
        ImGui::BeginGroup();
        DrawViewSettings(Client);

        if (Editor::EditorStyle::IconButton("Transform Tools", Editor::EditorStyle::EIcon::Tools, ImGui::IsPopupOpen("TransformTools"), true, true)) {
            ImGui::OpenPopup("TransformTools");
        }

        ImGui::SetNextWindowPos(ImVec2{ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 4.0f}, ImGuiCond_Appearing);

        if (ImGui::BeginPopup("TransformTools")) {
            ImGui::SeparatorText("Transform Tools");
            DrawModeCommands(Client, true);
            ImGui::Separator();

            if (ImGui::BeginMenu("Coordinate System", Client->CanExecuteCommand(Editor::EViewportCommand::WorldSpace) || Client->CanExecuteCommand(Editor::EViewportCommand::LocalSpace))) {
                DrawCoordinateMenu(Client);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Grid Settings")) {
                DrawGridSettings(Client);
                ImGui::EndMenu();
            }

            ImGui::Separator();

            const Editor::FViewportCommandInfo* Cycle{Editor::FViewportCommandInfo::Find(Editor::EViewportCommand::CycleTransformMode)};
            const Core::FString Shortcut{Cycle->GetShortcutLabel()};
            ImGui::TextDisabled("%s: %s", Shortcut.c_str(), Cycle->mLabel);
            ImGui::EndPopup();
        }

        ImGui::SameLine(0.0f, 6.0f);
        DrawModeCommands(Client, false);
        ImGui::SameLine(0.0f, 6.0f);

        const bool WorldSpace{Client->IsCommandChecked(Editor::EViewportCommand::WorldSpace)};
        const char* CoordinateTooltip{Client->GetGizmoMode() == Editor::EGizmoMode::Scale ? "Local Space\nScaling uses local axes." : WorldSpace ? "World Space (click to switch)" : "Local Space (click to switch)"};

        if (Editor::EditorStyle::IconButton("Coordinate System", WorldSpace ? Editor::EditorStyle::EIcon::World : Editor::EditorStyle::EIcon::Local, false, Client->CanExecuteCommand(Editor::EViewportCommand::ToggleCoordinateSystem), false, CoordinateTooltip)) {
            Client->ExecuteCommand(Editor::EViewportCommand::ToggleCoordinateSystem);
        }

        ImGui::SameLine(0.0f, 6.0f);

        if (Editor::EditorStyle::IconButton("Grid Settings", Editor::EditorStyle::EIcon::Grid, ImGui::IsPopupOpen("GridSettings"), true, true)) {
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
            ImGui::BeginDisabled(!Client->CanEditGizmoSettings());

            const Editor::FGizmoSettings* Settings{Client->GetGizmoSettings()};

            DrawSettingToggle(Client, Editor::EViewportCommand::ToggleTranslationSnap);
            DrawNumericSetting(Client, Editor::EGizmoSetting::TranslationStep, "Translation Step", Settings->mTranslationStep);
            DrawSettingToggle(Client, Editor::EViewportCommand::ToggleRotationSnap);
            DrawNumericSetting(Client, Editor::EGizmoSetting::RotationStep, "Rotation Degrees", Settings->mRotationStep);
            DrawSettingToggle(Client, Editor::EViewportCommand::ToggleScaleSnap);
            DrawNumericSetting(Client, Editor::EGizmoSetting::ScaleStep, "Scale Step", Settings->mScaleStep);
            DrawNumericSetting(Client, Editor::EGizmoSetting::Size, "Gizmo Size", Settings->mSize);
            ImGui::EndDisabled();
            ImGui::EndPopup();
        }

        ImGui::EndGroup();
        Editor::EditorStyle::PopViewportToolbar();
        ImGui::PopID();
    }

}
