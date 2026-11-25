/**
 * @file ViewportToolbar.cpp
 * @brief 공통 스타일과 뷰포트 클라이언트 명령으로 변환 도구 툴바를 구성합니다.
 * @ingroup Editor
 */

#include "Core/Types.h"
#include "Core/Literals/AngleFunctions.h"
#include "ViewportToolbar.h"
#include "BufferVisualization.h"
#include "EditorStyle.h"
#include "Editor/FEditorViewportClient.h"
#include "Editor/FViewportCommandInfo.h"
#include "Editor/Widget/FEditorQuantityInputWidget.h"

#include <algorithm>
#include <format>
#include <span>

using namespace Core::Literals;

namespace Editor {

    /**
     * @brief 툴바와 메뉴에서 공유하는 변환 명령 목록과 표시 보조 함수를 제공합니다.
     */
    namespace {
        constexpr Editor::EditorStyle::EIcon CameraIcon{Editor::EditorStyle::EIcon::Camera};
        constexpr Editor::EditorStyle::EIcon CameraSpeedIcon{Editor::EditorStyle::EIcon::CameraSpeed};
        constexpr Editor::EditorStyle::EIcon ViewModeIcon{Editor::EditorStyle::EIcon::Material};
        constexpr Core::TStaticArray<Engine::EViewMode, 5> ColorModes{Engine::EViewMode::Lit_Gouraud, Engine::EViewMode::Lit_Lambert, Engine::EViewMode::Lit_Phong, Engine::EViewMode::Unlit, Engine::EViewMode::Wireframe};
        constexpr Core::TStaticArray<const char*, ColorModes.size()> ColorModeNames{"Lit (Gouraud)", "Lit (Lambert)", "Lit", "Unlit", "Wireframe"};
        constexpr char PhysicsBodiesName[]{"Physics Bodies"};
        constexpr Core::TStaticArray<const char*, 7> ViewTypeNames{"Perspective", "Top", "Bottom", "Front", "Back", "Left", "Right"};

        struct FSnapControl {
            Editor::EViewportCommand mCommand{};
            Editor::EGizmoSetting mSetting{};
            const char* mTooltip{};
            const char* mSuffix{};
            std::span<const float> mPresets{};
        };

        constexpr Core::TStaticArray<float, 12> TranslationPresets{(0.01_m).GetBase(), (0.05_m).GetBase(), (0.1_m).GetBase(), (0.25_m).GetBase(), (0.5_m).GetBase(), (1_m).GetBase(), (2_m).GetBase(), (5_m).GetBase(), (10_m).GetBase(), (25_m).GetBase(), (50_m).GetBase(), (100_m).GetBase()};
        constexpr Core::TStaticArray<float, 8> RotationPresets{Core::Math::ToDegrees(1_deg), Core::Math::ToDegrees(5_deg), Core::Math::ToDegrees(10_deg), Core::Math::ToDegrees(15_deg), Core::Math::ToDegrees(30_deg), Core::Math::ToDegrees(45_deg), Core::Math::ToDegrees(60_deg), Core::Math::ToDegrees(90_deg)};
        constexpr Core::TStaticArray<float, 8> ScalePresets{0.01f, 0.025f, 0.05f, 0.1f, 0.125f, 0.25f, 0.5f, 1.0f};
        constexpr Core::TStaticArray<FSnapControl, 3> SnapControls{
            FSnapControl{Editor::EViewportCommand::ToggleTranslationSnap, Editor::EGizmoSetting::TranslationStep, "이동 스냅", " m", TranslationPresets},
            FSnapControl{Editor::EViewportCommand::ToggleRotationSnap, Editor::EGizmoSetting::RotationStep, "회전 스냅", "°", RotationPresets},
            FSnapControl{Editor::EViewportCommand::ToggleScaleSnap, Editor::EGizmoSetting::ScaleStep, "스케일 스냅", "", ScalePresets}
        };

        bool BeginToolbarPopup(const char* Id, bool Open) {
            if (Open) {
                ImGui::OpenPopup(Id);
            }

            const float Scale{ImGui::GetStyle().FontScaleMain * ImGui::GetStyle().FontScaleDpi};
            ImGui::SetNextWindowPos({ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y + 4.0f * Scale}, ImGuiCond_Appearing);

            return Editor::EditorStyle::BeginPopup(Id);
        }

        void DrawPlaceholder(const char* Label, Editor::EditorStyle::EIcon Icon, bool Dropdown) {
            const Core::FString Tooltip{std::format("{} ( 준비중 )", Label)};
            Editor::EditorStyle::IconButton(Label, Icon, false, false, Dropdown, Tooltip.c_str());
        }

        /**
         * @brief 동일한 변환 명령 목록을 아이콘 버튼 또는 메뉴 항목으로 표시합니다.
         * @param[in,out] Client 명령 실행과 상태 조회에 사용할 뷰포트 클라이언트입니다.
         * @param[in] Menu true이면 메뉴 항목으로 표시하며 false이면 한 줄의 버튼으로 표시합니다.
         */
        void DrawModeCommands(Editor::FEditorViewportClient* Client, bool Menu) {
            bool Drawn{};

            for (const Editor::FViewportCommandInfo& Info : Editor::FViewportCommandInfo::GetCommands()) {
                if (!Info.mTransformMode) {
                    continue;
                }

                if (!Menu && Drawn) {
                    ImGui::SameLine();
                }

                const Editor::EViewportCommand Command{Info.mCommand};
                const bool Selected{Client->IsCommandChecked(Command)};
                const bool Enabled{Client->CanExecuteCommand(Command)};
                const Core::FString Shortcut{Info.GetShortcutLabel()};
                const char* TooltipName{Command == Editor::EViewportCommand::Select ? "선택" : Command == Editor::EViewportCommand::Translate ? "이동" : Command == Editor::EViewportCommand::Rotate ? "회전" : Command == Editor::EViewportCommand::Scale ? "크기 조절" : Info.mLabel};
                const Core::FString Tooltip{Core::FString{TooltipName} + " (" + Shortcut + ")"};
                const Editor::EditorStyle::EIcon ButtonIcon{Command == Editor::EViewportCommand::Select ? Editor::EditorStyle::EIcon::Select : Info.mIcon};
                const bool Pressed{Menu ? Command == Editor::EViewportCommand::Select ? ImGui::MenuItem(Info.mLabel, Shortcut.c_str(), Selected, Enabled) : Editor::EditorStyle::IconMenuItem(Info.mLabel, Info.mIcon, Selected, Enabled) : Editor::EditorStyle::IconButton(Info.mLabel, ButtonIcon, Selected, Enabled, false, Tooltip.c_str(), 0.0f, Editor::EditorStyle::EIconSelectionStyle::Icon)};

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
         * @brief 기즈모 수치 설정의 입력을 표시하고 변경 값을 클라이언트의 검증 경로로 전달합니다.
         * @param[in,out] Client 변경 값을 검증하고 적용할 유효한 클라이언트입니다.
         * @param[in] Setting 변경할 수치 항목입니다.
         * @param[in] Label 현재 UI 범위에서 입력을 식별할 표시 이름입니다.
         * @param[in] Value 현재 설정 값이며 입력 중에는 지역 사본만 수정합니다.
         * @note 크기는 슬라이더로, 나머지는 수치 입력으로 표시하며 기즈모 설정의 주소를 UI에 직접 전달하지 않습니다.
         */
        void DrawNumericSetting(Editor::FEditorViewportClient* Client, Editor::EGizmoSetting Setting, const char* Label, float Value) {
            const bool Changed{Setting == Editor::EGizmoSetting::Size ? ImGui::SliderFloat(Label, &Value, 0.5f, 2.0f) : ImGui::InputFloat(Label, &Value, 0.0f, 0.0f, "%.8g")};

            if (Changed) {
                Client->SetGizmoSetting(Setting, Value);
            }
        }

        void DrawSnapValues(Editor::FEditorViewportClient* Client, const FSnapControl& Control) {
            const float Current{Client->GetGizmoSettingValue(Control.mSetting)};

            for (const float Value : Control.mPresets) {
                const Core::FString Label{std::format("{:g}{}", Value, Control.mSuffix)};

                if (ImGui::MenuItem(Label.c_str(), nullptr, Current == Value)) {
                    Client->SetGizmoSetting(Control.mSetting, Value);
                }
            }

            ImGui::Separator();

            if (Control.mSetting == Editor::EGizmoSetting::TranslationStep) {
                Editor::FEditorQuantityInputWidget Input{"직접 입력", Client->GetGizmoSettings()->mTranslationStep.GetBase(), Core::QuantityUnits::Meter};
                Input.Draw(0.0f);

                if (const auto Value{Input.GetEditedValue()}; Value.has_value()) {
                    Client->SetTranslationSnapStep(Core::FLength::FromBase(*Value));
                }
            } else if (Control.mSetting == Editor::EGizmoSetting::RotationStep) {
                Editor::FEditorQuantityInputWidget Input{"직접 입력", Client->GetGizmoSettings()->mRotationStep.GetBase(), Core::QuantityUnits::Degree};
                Input.Draw(0.0f);

                if (const auto Value{Input.GetEditedValue()}; Value.has_value()) {
                    Client->SetRotationSnapStep(Core::FAngle::FromBase(*Value));
                }
            } else {
                DrawNumericSetting(Client, Control.mSetting, "직접 입력", Current);
            }
        }

        void DrawSnapButton(Editor::FEditorViewportClient* Client, const FSnapControl& Control, const char* Label) {
            const Editor::FViewportCommandInfo* Info{Editor::FViewportCommandInfo::Find(Control.mCommand)};
            const bool Enabled{Client->CanExecuteCommand(Control.mCommand)};
            const bool Checked{Client->IsCommandChecked(Control.mCommand)};
            ImGui::BeginDisabled(!Enabled);
            const auto Pressed{Editor::EditorStyle::SplitIconButton(Info->mLabel, Info->mIcon, Control.mTooltip, "스냅 간격", {.mMenuLabel{Label}, .mActionChecked{Checked}})};
            ImGui::EndDisabled();

            if (Pressed == Editor::EditorStyle::ESplitIconPress::Action) {
                Client->ExecuteCommand(Control.mCommand);
            }

            if (BeginToolbarPopup(Info->mLabel, Pressed == Editor::EditorStyle::ESplitIconPress::Menu)) {
                ImGui::BeginDisabled(!Enabled);
                DrawSnapValues(Client, Control);
                ImGui::EndDisabled();
                Editor::EditorStyle::EndPopup();
            }
        }

        void DrawTransformSettings(Editor::FEditorViewportClient* Client) {
            DrawCoordinateMenu(Client);
            ImGui::Separator();
            ImGui::BeginDisabled(!Client->CanEditGizmoSettings());
            DrawNumericSetting(Client, Editor::EGizmoSetting::Size, "Gizmo Size", Client->GetGizmoSettings()->mSize);
            ImGui::EndDisabled();
        }

        void DrawViewTypeMenu(Editor::FEditorViewportClient* Client, const std::function<void(Editor::EEditorViewportType)>& SetViewType) {
            for (Index_t Index{}; Index < ViewTypeNames.size(); ++Index) {
                const auto Type{static_cast<Editor::EEditorViewportType>(Index)};

                if (Index == 1) {
                    ImGui::SeparatorText("Orthographic");
                }

                if (ImGui::MenuItem(ViewTypeNames[Index], nullptr, Client->GetViewType() == Type)) {
                    if (SetViewType) {
                        SetViewType(Type);
                    } else {
                        Client->SetViewType(Type);
                    }
                }
            }
        }

        Index_t GetBufferModeIndex(Engine::EViewMode Mode) {
            const auto& Modes{Editor::BufferVisualization::Modes};
            const auto Current{std::find(Modes.begin(), Modes.end(), Mode)};

            return Current == Modes.end() ? 0u : static_cast<Index_t>(Current - Modes.begin());
        }

        const char* GetViewModeName(Editor::FEditorViewportClient* Client) {
            if (Client->GetViewMode() == Engine::EViewMode::PhysicsBodies) {
                return PhysicsBodiesName;
            }

            const Index_t Buffer{GetBufferModeIndex(Client->GetViewMode())};

            if (Buffer != 0) {
                return Editor::BufferVisualization::Names[Buffer];
            }

            const auto Current{std::find(ColorModes.begin(), ColorModes.end(), Client->GetColorViewMode())};

            return Current == ColorModes.end() ? "Lit" : ColorModeNames[Current - ColorModes.begin()];
        }

        void DrawViewModeMenu(Editor::FEditorViewportClient* Client) {
            for (Index_t Mode{}; Mode < ColorModes.size(); ++Mode) {
                if (ImGui::MenuItem(ColorModeNames[Mode], nullptr, Client->GetViewMode() == ColorModes[Mode])) {
                    Client->SetViewMode(ColorModes[Mode]);
                }
            }

            if (ImGui::MenuItem(PhysicsBodiesName, nullptr, Client->GetViewMode() == Engine::EViewMode::PhysicsBodies)) {
                Client->SetViewMode(Engine::EViewMode::PhysicsBodies);
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("Buffer Visualization")) {
                const Index_t Current{GetBufferModeIndex(Client->GetViewMode())};
                const auto& Modes{Editor::BufferVisualization::Modes};
                const auto& Names{Editor::BufferVisualization::Names};

                for (Index_t Buffer{}; Buffer < Modes.size(); ++Buffer) {
                    if (ImGui::MenuItem(Names[Buffer], nullptr, Buffer == Current, Client->SupportsViewMode(Modes[Buffer]))) {
                        Client->SetViewMode(Buffer == 0 ? Client->GetColorViewMode() : Modes[Buffer]);
                    }
                }

                ImGui::EndMenu();
            }
        }

        void DrawShowFlags(Editor::FEditorViewportClient* Client) {
            ImGui::SeparatorText("Show Flags");

            Engine::FEngineShowFlags Flags{Client->GetShowFlags()};
            bool Changed{ImGui::Checkbox("Primitives", &Flags.mPrimitives)};

            Changed = ImGui::Checkbox("Sprites / Billboards", &Flags.mSprites) || Changed;
            Changed = ImGui::Checkbox("Text", &Flags.mText) || Changed;
            Changed = ImGui::Checkbox("Light Icons", &Flags.mLightIcons) || Changed;
            Changed = ImGui::Checkbox("Shadows", &Flags.mShadows) || Changed;
            Changed = ImGui::Checkbox("Sky", &Flags.mSky) || Changed;
            Changed = ImGui::Checkbox("Fog", &Flags.mFog) || Changed;
            Changed = ImGui::Checkbox("Depth of Field", &Flags.mDepthOfField) || Changed;
            Changed = ImGui::Checkbox("FXAA", &Flags.mFXAA) || Changed;
            Changed = ImGui::Checkbox("Fog Icons", &Flags.mFogIcons) || Changed;
            Changed = ImGui::Checkbox("Light Visualizers", &Flags.mLightVisualizers) || Changed;
            Changed = ImGui::Checkbox("Bounding Boxes (AABB)", &Flags.mBounds) || Changed;
            Changed = ImGui::Checkbox("Collision", &Flags.mCollision) || Changed;
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
        }

        void DrawCameraSpeed(Editor::FEditorViewportClient* Client) {
            float Speed{Client->GetCameraSpeed().GetBase()};

            if (ImGui::SliderFloat("Movement Speed", &Speed, (0.1_mps).GetBase(), (128_mps).GetBase(), "%.2f m/s", ImGuiSliderFlags_Logarithmic)) {
                Client->SetCameraSpeed(Core::FSpeed::FromBase(Speed));
            }
        }

        void DrawViewportSettings(Editor::FEditorViewportClient* Client) {
            ImGui::SeparatorText("Camera Settings");

            float Sensitivity{Client->GetCameraSensitivity()};

            if (ImGui::SliderFloat("Sensitivity", &Sensitivity, Core::Math::ToDegrees(0.01_deg), Core::Math::ToDegrees(2_deg), "%.2f")) {
                Client->SetCameraSensitivity(Sensitivity);
            }

            Engine::FEditorCameraState Camera{Client->GetPerspectiveCamera()};
            float FieldOfView{Core::Math::ToDegrees(Camera.GetFieldOfView())};
            float NearClip{Camera.GetNearClip().GetBase()};
            float FarClip{Camera.GetFarClip().GetBase()};
            bool ProjectionChanged{ImGui::SliderFloat("FOV", &FieldOfView, Core::Math::ToDegrees(1_deg), Core::Math::ToDegrees(179_deg), "%.1f")};

            ProjectionChanged = ImGui::InputFloat("Near Clip", &NearClip) || ProjectionChanged;
            ProjectionChanged = ImGui::InputFloat("Far Clip", &FarClip) || ProjectionChanged;

            if (ProjectionChanged && Camera.Set(Camera.GetLocation(), Camera.GetRotation(), FieldOfView * 1_deg, Core::FLength::FromBase(NearClip), Core::FLength::FromBase(FarClip))) {
                Client->SetPerspectiveCamera(Camera);
            }

            DrawGridSettings(Client);
        }

        void DrawCompactMenu(Editor::FEditorViewportClient* Client, const std::function<void(Editor::EEditorViewportType)>& SetViewType) {
            ImGui::Separator();
            DrawTransformSettings(Client);

            for (const FSnapControl& Control : SnapControls) {
                if (ImGui::BeginMenu(Control.mTooltip, Client->CanExecuteCommand(Control.mCommand))) {
                    if (ImGui::MenuItem("활성화", nullptr, Client->IsCommandChecked(Control.mCommand))) {
                        Client->ExecuteCommand(Control.mCommand);
                    }

                    DrawSnapValues(Client, Control);
                    ImGui::EndMenu();
                }
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("뷰포트 종류")) {
                DrawViewTypeMenu(Client, SetViewType);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("카메라 속도")) {
                DrawCameraSpeed(Client);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("뷰 모드")) {
                DrawViewModeMenu(Client);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("표시 플래그")) {
                DrawShowFlags(Client);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("뷰포트 설정")) {
                DrawViewportSettings(Client);
                ImGui::EndMenu();
            }
        }

        void DrawToolsButton(Editor::FEditorViewportClient* Client, bool Compact, const std::function<void(Editor::EEditorViewportType)>& SetViewType) {
            if (BeginToolbarPopup("TransformTools", Editor::EditorStyle::IconButton("Tools", Editor::EditorStyle::EIcon::Tools, false, true, true, Compact ? "뷰포트 도구" : "변환 도구"))) {
                DrawModeCommands(Client, true);

                if (Compact) {
                    DrawCompactMenu(Client, SetViewType);
                }

                Editor::EditorStyle::EndPopup();
            }
        }

        void DrawTransformControls(Editor::FEditorViewportClient* Client, const Core::TStaticArray<Core::FString, SnapControls.size()>& SnapLabels, float GroupSpacing, bool ShowPlaceholders) {
            ImGui::SameLine(0.0f, GroupSpacing);

            const bool WorldSpace{Client->IsCommandChecked(Editor::EViewportCommand::WorldSpace)};
            const char* CoordinateTooltip{Client->GetGizmoMode() == Editor::EGizmoMode::Scale ? "로컬 좌표계\n크기 조절은 로컬 축을 사용합니다." : WorldSpace ? "월드 좌표계 (클릭하면 전환)" : "로컬 좌표계 (클릭하면 전환)"};
            const auto Pressed{Editor::EditorStyle::SplitIconButton("CoordinateSystem", WorldSpace ? Editor::EditorStyle::EIcon::World : Editor::EditorStyle::EIcon::Local, CoordinateTooltip, "좌표계 및 기즈모 설정", {.mActionEnabled{Client->CanExecuteCommand(Editor::EViewportCommand::ToggleCoordinateSystem)}})};

            if (Pressed == Editor::EditorStyle::ESplitIconPress::Action) {
                Client->ExecuteCommand(Editor::EViewportCommand::ToggleCoordinateSystem);
            }

            if (BeginToolbarPopup("TransformSettings", Pressed == Editor::EditorStyle::ESplitIconPress::Menu)) {
                DrawTransformSettings(Client);
                Editor::EditorStyle::EndPopup();
            }

            ImGui::SameLine(0.0f, GroupSpacing);

            if (ShowPlaceholders) {
                DrawPlaceholder("오브젝트 스냅 설정", Editor::EditorStyle::EIcon::Snapping, true);
                ImGui::SameLine();
                DrawPlaceholder("표면 스냅", Editor::EditorStyle::EIcon::SurfaceSnap, true);
                ImGui::SameLine();
            }

            for (Index_t Index{}; Index < SnapControls.size(); ++Index) {
                if (Index != 0) {
                    ImGui::SameLine();
                }

                DrawSnapButton(Client, SnapControls[Index], SnapLabels[Index].c_str());
            }
        }

        void DrawViewTypeButton(Editor::FEditorViewportClient* Client, const char* Label, const std::function<void(Editor::EEditorViewportType)>& SetViewType) {
            const bool Open{Label != nullptr ? Editor::EditorStyle::LabelButton(Label, &CameraIcon, true, true, "뷰포트 종류") : Editor::EditorStyle::IconButton("ViewType", CameraIcon, false, true, true, "뷰포트 종류")};

            if (BeginToolbarPopup("ViewTypeMenu", Open)) {
                DrawViewTypeMenu(Client, SetViewType);
                Editor::EditorStyle::EndPopup();
            }
        }

        void DrawViewControls(Editor::FEditorViewportClient* Client, const char* ViewModeLabel, const char* SpeedLabel, float GroupSpacing, bool ShowPlaceholders) {
            ImGui::SameLine();

            if (BeginToolbarPopup("CameraSpeed", Editor::EditorStyle::LabelButton(SpeedLabel, &CameraSpeedIcon, true, true, "카메라 속도", true))) {
                DrawCameraSpeed(Client);
                Editor::EditorStyle::EndPopup();
            }

            ImGui::SameLine(0.0f, GroupSpacing);

            if (BeginToolbarPopup("ViewModeMenu", Editor::EditorStyle::LabelButton(ViewModeLabel, &ViewModeIcon, true, true, "뷰 모드"))) {
                DrawViewModeMenu(Client);
                Editor::EditorStyle::EndPopup();
            }

            ImGui::SameLine();

            if (BeginToolbarPopup("ShowMenu", Editor::EditorStyle::IconButton("Show", Editor::EditorStyle::EIcon::Visible, false, true, true, "표시 플래그"))) {
                DrawShowFlags(Client);
                Editor::EditorStyle::EndPopup();
            }

            if (ShowPlaceholders) {
                ImGui::SameLine();
                DrawPlaceholder("뷰포트 성능 및 렌더링 품질 설정", Editor::EditorStyle::EIcon::Performance, true);
                ImGui::SameLine();
                DrawPlaceholder("뷰포트 품질 경고 및 기본값 복원", Editor::EditorStyle::EIcon::Warning, false);
            }

            ImGui::SameLine(0.0f, GroupSpacing);

            if (BeginToolbarPopup("ViewportSettings", Editor::EditorStyle::IconButton("Settings", Editor::EditorStyle::EIcon::Settings, false, true, true, "뷰포트 설정"))) {
                DrawViewportSettings(Client);
                Editor::EditorStyle::EndPopup();
            }
        }

        void DrawControls(Editor::FEditorViewportClient* Client, const ImVec2& Start, float Width, float Scale, const std::function<void(Editor::EEditorViewportType)>& SetViewType, const std::function<void()>& DrawLayout) {
            const float IconWidth{ImGui::GetFrameHeight()};
            const float DropdownWidth{IconWidth + 12.0f * Scale};
            const float SplitWidth{Editor::EditorStyle::GetSplitIconButtonWidth()};
            const float Spacing{ImGui::GetStyle().ItemSpacing.x};
            const float GroupSpacing{8.0f * Scale};
            const float LayoutWidth{DrawLayout ? SplitWidth + Spacing : 0.0f};
            const Core::FString ViewTypeLabel{Core::FString{Client->GetViewTypeName()} + "###ViewType"};
            const Core::FString ViewModeLabel{Core::FString{GetViewModeName(Client)} + "###ViewMode"};
            const Core::FString SpeedLabel{std::format("{:.3g}###CameraSpeed", Client->GetCameraSpeed().GetBase())};
            Core::TStaticArray<Core::FString, SnapControls.size()> SnapLabels{};
            const float ModesWidth{4.0f * (IconWidth + Spacing)};
            const float TransformPlaceholderWidth{2.0f * (DropdownWidth + Spacing)};
            const float ViewPlaceholderWidth{DropdownWidth + IconWidth + 2.0f * Spacing};
            float TransformWidth{4.0f * IconWidth + DropdownWidth + SplitWidth + 6.0f * Spacing + 2.0f * GroupSpacing};
            float ViewWidth{2.0f * DropdownWidth + 2.0f * Spacing + 2.0f * GroupSpacing};

            for (Index_t Index{}; Index < SnapControls.size(); ++Index) {
                const FSnapControl& Control{SnapControls[Index]};
                SnapLabels[Index] = std::format("{:.4g}{}", Client->GetGizmoSettingValue(Control.mSetting), Control.mSuffix);
                TransformWidth += Editor::EditorStyle::GetSplitIconButtonWidth(SnapLabels[Index].c_str());
            }

            ViewWidth += Editor::EditorStyle::GetLabelButtonWidth(ViewTypeLabel.c_str(), &CameraIcon, true);
            ViewWidth += Editor::EditorStyle::GetLabelButtonWidth(ViewModeLabel.c_str(), &ViewModeIcon, true);
            ViewWidth += Editor::EditorStyle::GetLabelButtonWidth(SpeedLabel.c_str(), &CameraSpeedIcon, true);

            ViewWidth += LayoutWidth;

            const float ControlsWidth{TransformWidth + ViewWidth + GroupSpacing};
            const bool ShowPlaceholders{Width >= ControlsWidth + TransformPlaceholderWidth + ViewPlaceholderWidth};
            const bool Compact{Width < ControlsWidth};
            const bool ShowViewType{!Compact || Width >= 2.0f * DropdownWidth + LayoutWidth + GroupSpacing};
            const bool ShowLayout{DrawLayout && (!Compact || Width >= DropdownWidth + LayoutWidth)};

            if (ShowPlaceholders) {
                ViewWidth += ViewPlaceholderWidth;
            } else if (Compact) {
                ViewWidth = (ShowLayout ? SplitWidth : 0.0f) + (ShowViewType ? DropdownWidth : 0.0f) + (ShowLayout && ShowViewType ? Spacing : 0.0f);
            }

            DrawToolsButton(Client, Compact, SetViewType);

            if (!Compact || Width >= DropdownWidth + ModesWidth + ViewWidth + GroupSpacing) {
                ImGui::SameLine();
                DrawModeCommands(Client, false);
            }

            if (!Compact) {
                DrawTransformControls(Client, SnapLabels, GroupSpacing, ShowPlaceholders);
            }

            if (ViewWidth > 0.0f) {
                ImGui::SetCursorScreenPos({Start.x + Width - ViewWidth, Start.y});

                if (ShowViewType) {
                    DrawViewTypeButton(Client, Compact ? nullptr : ViewTypeLabel.c_str(), SetViewType);
                }

                if (!Compact) {
                    DrawViewControls(Client, ViewModeLabel.c_str(), SpeedLabel.c_str(), GroupSpacing, ShowPlaceholders);
                }

                if (ShowLayout) {
                    if (ShowViewType) {
                        ImGui::SameLine();
                    }

                    DrawLayout();
                }
            }
        }
    }

    void Editor::ViewportToolbar::Draw(Editor::FEditorViewportClient* Client, float Width, bool AllowInput, const char* PlayStatus, const std::function<void(Editor::EEditorViewportType)>& SetViewType, const std::function<void()>& DrawLayout) {
        ImGui::PushID("ViewportToolbar");
        Editor::EditorStyle::PushViewportToolbar();

        const float Scale{ImGui::GetStyle().FontScaleMain * ImGui::GetStyle().FontScaleDpi};
        const float Padding{4.0f * Scale};
        const float Height{ImGui::GetFrameHeight() + 2.0f * Padding};
        const ImVec2 Start{ImGui::GetCursorScreenPos()};
        const ImVec2 End{Start.x + Width, Start.y + Height};
        ImDrawList* DrawList{ImGui::GetWindowDrawList()};

        ImGui::Dummy({Width, Height});
        DrawList->AddRectFilled(Start, End, ImGui::GetColorU32(Editor::EditorStyle::GetTopLevelTabColor()));
        DrawList->AddLine({Start.x, End.y - 1.0f}, {End.x, End.y - 1.0f}, ImGui::GetColorU32(ImGuiCol_Separator));

        if (Width > 2.0f * Padding) {
            const ImVec2 Content{Start.x + Padding, Start.y + Padding};
            ImGui::SetCursorScreenPos(Content);

            if (PlayStatus != nullptr) {
                const bool PhysicsBodies{Client->GetViewMode() == Engine::EViewMode::PhysicsBodies};

                ImGui::BeginDisabled(!AllowInput);

                if (Editor::EditorStyle::LabelButton(PhysicsBodies ? PhysicsBodiesName : "Lit", &ViewModeIcon, true, false, "Toggle Physics Bodies")) {
                    Client->SetViewMode(PhysicsBodies ? Client->GetColorViewMode() : Engine::EViewMode::PhysicsBodies);
                }

                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(PlayStatus);
            } else {
                ImGui::BeginDisabled(!AllowInput);
                DrawControls(Client, Content, Width - 2.0f * Padding, Scale, SetViewType, DrawLayout);
                ImGui::EndDisabled();
            }
        }

        Editor::EditorStyle::PopViewportToolbar();
        ImGui::PopID();
        ImGui::SetCursorScreenPos({Start.x, End.y});
    }

}
