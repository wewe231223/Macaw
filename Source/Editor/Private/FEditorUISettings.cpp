/**
 * @file FEditorUISettings.cpp
 * @brief 프로젝트별 에디터 환경 설정의 저장·복원과 테마·뷰포트·렌더링 설정 UI를 구현합니다.
 * @ingroup Editor
 */

#include "FEditorUI.h"
#include "Core/FilePath.h"
#include "Core/Stats.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorViewport.h"
#include "Editor/FPlayInEditorSession.h"
#include "Engine/FProject.h"
#include "Platform/Window.h"
#include "RenderCore/IRendererModule.h"
#include "Serialization/JsonArchive.h"

#include <imgui_internal.h>
#include <commdlg.h>
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <utility>

namespace Editor {

    /**
     * @brief 이 구현 파일에서만 사용하는 검증·변환·설정 보조 함수를 제공합니다.
     */
    namespace {
        /**
         * @brief 뷰포트 설정 UI와 직렬화에서 공유하는 표시 이름·ShowFlag 멤버 포인터 목록입니다.
         */
        const std::array<std::pair<const char*, bool Engine::FEngineShowFlags::*>, 12> ShowOptions{{
            {"Primitives", &Engine::FEngineShowFlags::mPrimitives},
            {"Sprites", &Engine::FEngineShowFlags::mSprites},
            {"Text", &Engine::FEngineShowFlags::mText},
            {"Shadows", &Engine::FEngineShowFlags::mShadows},
            {"Fog", &Engine::FEngineShowFlags::mFog},
            {"FXAA", &Engine::FEngineShowFlags::mFXAA},
            {"Fog Icons", &Engine::FEngineShowFlags::mFogIcons},
            {"Light Icons", &Engine::FEngineShowFlags::mLightIcons},
            {"Light Visualizers", &Engine::FEngineShowFlags::mLightVisualizers},
            {"Bounds", &Engine::FEngineShowFlags::mBounds},
            {"Grid", &Engine::FEngineShowFlags::mGrid},
            {"Axes", &Engine::FEngineShowFlags::mAxes}
        }};

        /**
         * @brief 버퍼 시각화를 제외한 컬러 조명·와이어프레임 표시 모드 목록입니다.
         */
        const std::array<Engine::EViewMode, 5> ViewModes{Engine::EViewMode::Lit_Gouraud, Engine::EViewMode::Lit_Lambert, Engine::EViewMode::Lit_Phong, Engine::EViewMode::Unlit, Engine::EViewMode::Wireframe};
        /**
         * @brief 컬러 표시 모드 목록과 같은 순서로 보여 줄 설정 UI 이름입니다.
         */
        const std::array<const char*, 5> ViewModeNames{"Lit - Gouraud", "Lit - Lambert", "Lit - Phong", "Unlit", "Wireframe"};

        /**
         * @brief 뷰포트 실수 설정을 드래그 위젯으로 표시하고 변경 시 검증용 Setter를 호출합니다.
         * @param[in] Label 설정 드래그 위젯에 표시할 이름입니다.
         * @param[in,out] Client 설정을 조회하고 변경할 뷰포트 클라이언트입니다.
         * @param[in] Getter 현재 설정값을 읽을 멤버 함수 포인터입니다.
         * @param[in] Setter 변경값을 검증하고 적용할 멤버 함수 포인터입니다.
         * @param[in] Minimum 위젯에 허용할 최솟값이며 입력 속도 산정에도 사용합니다.
         * @param[in] Maximum 허용할 최댓값 또는 최대 불투명도입니다.
         * @param[in] Format 실수 표시 형식을 지정할 printf 형식 문자열입니다.
         * @return 입력이 바뀌고 Setter가 변경을 수락하면 true입니다.
         */
        bool DrawClientFloat(const char* Label, Editor::FEditorViewportClient& Client, float (Editor::FEditorViewportClient::*Getter)() const, bool (Editor::FEditorViewportClient::*Setter)(float), float Minimum, float Maximum, const char* Format) {
            float Value{(Client.*Getter)()};
            return ImGui::DragFloat(Label, &Value, Minimum * 0.1f, Minimum, Maximum, Format, ImGuiSliderFlags_AlwaysClamp) && (Client.*Setter)(Value);
        }

        /**
         * @brief 카메라 조작·표시·변환 스냅·통계 그룹 설정을 뷰포트별로 저장하거나 복원합니다.
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         * @param[in,out] Client 설정을 조회하고 변경할 뷰포트 클라이언트입니다.
         * @param[in,out] Statistics 저장하거나 복원할 뷰포트 통계 그룹 설정입니다.
         */
        void SerializeViewport(Serialization::FJsonArchive& Archive, Editor::FEditorViewportClient& Client, Editor::FViewportStatistics& Statistics) {
            float Speed{Client.GetCameraSpeed()};
            float Sensitivity{Client.GetCameraSensitivity()};
            float Spacing{Client.GetGridSpacing()};
            float Thickness{Client.GetLineThickness()};
            float Width{Client.GetOrthographicWidth()};
            Core::Int32 Mode{static_cast<Core::Int32>(Client.GetViewMode())};
            Core::Int32 ColorMode{static_cast<Core::Int32>(Client.GetColorViewMode())};
            Editor::FGizmoSettings Gizmo{*Client.GetGizmoSettings()};
            Core::Int32 TransformMode{static_cast<Core::Int32>(Client.GetGizmoMode())};
            Core::Int32 Space{static_cast<Core::Int32>(Client.GetGizmoSpace())};
            Engine::FEngineShowFlags Flags{Client.GetShowFlags()};
            Archive.Serialize("CameraSpeed", Speed);
            Archive.Serialize("CameraSensitivity", Sensitivity);
            Archive.Serialize("GridSpacing", Spacing);
            Archive.Serialize("LineThickness", Thickness);
            Archive.Serialize("OrthographicWidth", Width);
            Archive.Serialize("ViewMode", Mode);

            if (!Archive.IsLoading() || Archive.HasField("ColorViewMode")) {
                Archive.Serialize("ColorViewMode", ColorMode);
            }

            Archive.Serialize("TranslationSnap", Gizmo.mTranslationSnap);
            Archive.Serialize("RotationSnap", Gizmo.mRotationSnap);
            Archive.Serialize("ScaleSnap", Gizmo.mScaleSnap);
            Archive.Serialize("TranslationStep", Gizmo.mTranslationStep);
            Archive.Serialize("RotationStep", Gizmo.mRotationStep);
            Archive.Serialize("ScaleStep", Gizmo.mScaleStep);
            Archive.Serialize("GizmoSize", Gizmo.mSize);
            Archive.Serialize("TransformMode", TransformMode);
            Archive.Serialize("CoordinateSpace", Space);

            for (const auto& [Name, Member] : ShowOptions) {
                if (!Archive.IsLoading() || Archive.HasField(Name)) {
                    Archive.Serialize(Name, Flags.*Member);
                }
            }

            Core::TArray<Core::FString> Groups{Core::FStats::Get().GetGroups()};
            Core::FString EnabledGroups{"\n"};

            for (const Core::FString& Group : Groups) {
                if (Statistics.IsEnabled(Group)) {
                    EnabledGroups += Group + '\n';
                }
            }

            Archive.Serialize("StatGroups", EnabledGroups);

            if (!Archive.IsLoading() || Archive.HasError()) {
                return;
            }

            Client.SetCameraSpeed(Speed);
            Client.SetCameraSensitivity(Sensitivity);
            Client.SetGridSpacing(Spacing);
            Client.SetLineThickness(Thickness);
            Client.SetOrthographicWidth(Width);
            Client.SetViewMode(static_cast<Engine::EViewMode>(ColorMode));
            Client.SetViewMode(static_cast<Engine::EViewMode>(Mode));
            Client.SetShowFlags(Flags);
            Client.SetGizmoSetting(Editor::EGizmoSetting::TranslationStep, Gizmo.mTranslationStep);
            Client.SetGizmoSetting(Editor::EGizmoSetting::RotationStep, Gizmo.mRotationStep);
            Client.SetGizmoSetting(Editor::EGizmoSetting::ScaleStep, Gizmo.mScaleStep);
            Client.SetGizmoSetting(Editor::EGizmoSetting::Size, Gizmo.mSize);

            for (const auto& [Command, Enabled] : {std::pair{Editor::EViewportCommand::ToggleTranslationSnap, Gizmo.mTranslationSnap}, std::pair{Editor::EViewportCommand::ToggleRotationSnap, Gizmo.mRotationSnap}, std::pair{Editor::EViewportCommand::ToggleScaleSnap, Gizmo.mScaleSnap}}) {
                if (Client.IsCommandChecked(Command) != Enabled) {
                    Client.ExecuteCommand(Command);
                }
            }

            const std::array<Editor::EViewportCommand, 4> Commands{Editor::EViewportCommand::Translate, Editor::EViewportCommand::Rotate, Editor::EViewportCommand::Scale, Editor::EViewportCommand::Select};

            if (TransformMode >= 0 && static_cast<size_t>(TransformMode) < Commands.size()) {
                Client.ExecuteCommand(Commands[TransformMode]);
            }

            if (Space == static_cast<Core::Int32>(Editor::EGizmoSpace::World) || Space == static_cast<Core::Int32>(Editor::EGizmoSpace::Local)) {
                Client.ExecuteCommand(Space == static_cast<Core::Int32>(Editor::EGizmoSpace::World) ? Editor::EViewportCommand::WorldSpace : Editor::EViewportCommand::LocalSpace);
            }

            Core::FString Output{};
            Statistics.ExecuteCommand("stat none", Output);

            for (const Core::FString& Group : Groups) {
                if (EnabledGroups.find('\n' + Group + '\n') != Core::FString::npos) {
                    Statistics.ExecuteCommand("stat " + Group + " on", Output);
                }
            }
        }
    }

    void Editor::FEditorUI::InitializeSettings() {
        mThemes = Editor::FImGuiThemeCatalog{};
        mThemeIndex = 0;
        mFontSize = 16.0f;
        mUIScale.SetUserScale(0.0f);
        mSettingsPath = mContext->GetProject()->GetRootPath() / "Saved/EditorSettings.json";
        mSettingsStatus.clear();
        mSettingsDirty = false;
        mSettingsSaveDelay = 0.0f;
        mSettingsViewport = 0;
        mThemeFilter.fill('\0');
        strncpy_s(mStartupScene.data(), mStartupScene.size(), mContext->GetProject()->GetStartupScene().c_str(), _TRUNCATE);

        std::array<wchar_t, 32768> Executable{};
        const DWORD Length{GetModuleFileNameW(nullptr, Executable.data(), static_cast<DWORD>(Executable.size()))};
        std::filesystem::path Bundled{Length > 0 && Length < Executable.size() ? std::filesystem::path{Executable.data()}.parent_path() / "Templates/Project/Editor/ImThemes/themes.toml" : std::filesystem::path{}};
        std::error_code Code{};

        if (!std::filesystem::is_regular_file(Bundled, Code)) {
            Bundled = "Templates/Project/Editor/ImThemes/themes.toml";
        }

        Core::FString Error{};

        if (!mThemes.Load(Bundled, Error)) {
            mContext->AddLog(Error);
            mSettingsStatus = Error;
        }

        const std::filesystem::path Imported{mSettingsPath.parent_path() / "Themes"};
        Core::TArray<std::filesystem::path> Files{};

        if (std::filesystem::is_directory(Imported, Code)) {
            for (std::filesystem::directory_iterator Iterator{Imported, Code}; !Code && Iterator != std::filesystem::directory_iterator{}; Iterator.increment(Code)) {
                if (Iterator->path().extension() == ".toml") {
                    Files.push_back(Iterator->path());
                }
            }
        }

        std::sort(Files.begin(), Files.end());

        for (const auto& File : Files) {
            if (!mThemes.Load(File, Error)) {
                mContext->AddLog(Error);
                mSettingsStatus = Error;
            }
        }

        LoadSettings();
        mAppearanceDirty = true;
    }

    void Editor::FEditorUI::LoadSettings() {
        std::error_code Code{};

        if (!std::filesystem::is_regular_file(mSettingsPath, Code)) {
            return;
        }

        Serialization::FJsonArchive Archive{Core::EArchiveMode::Loading};

        if (!Archive.Load(mSettingsPath)) {
            mSettingsStatus = Archive.GetError();
            mContext->AddLog(mSettingsStatus);
            return;
        }

        Core::Uint32 Version{};
        Archive.Serialize("Version", Version);

        if (Version != 1 || Archive.HasError()) {
            mSettingsStatus = "Cannot load unsupported editor settings";
            mContext->AddLog(mSettingsStatus);
            return;
        }

        SerializeSettings(Archive);

        if (Archive.HasError()) {
            mSettingsStatus = Archive.GetError();
            mContext->AddLog(mSettingsStatus);
        }
    }

    bool Editor::FEditorUI::SaveSettings() {
        if (mSettingsPath.empty()) {
            return false;
        }

        Serialization::FJsonArchive Archive{Core::EArchiveMode::Saving};
        Core::Uint32 Version{1};
        Archive.Serialize("Version", Version);
        SerializeSettings(Archive);

        std::error_code Code{};
        std::filesystem::create_directories(mSettingsPath.parent_path(), Code);
        std::filesystem::path Temporary{mSettingsPath};
        Temporary += ".tmp";

        if (Code || !Archive.Save(Temporary) || !MoveFileExW(Temporary.c_str(), mSettingsPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            mSettingsStatus = "Cannot save editor settings: " + (Code ? Code.message() : Archive.HasError() ? Archive.GetError() : std::system_category().message(static_cast<int>(GetLastError())));
            mContext->AddLog(mSettingsStatus);
            return false;
        }

        ImGui::SaveIniSettingsToDisk(mIniPath.c_str());
        mSettingsDirty = false;
        mSettingsSaveDelay = 0.0f;
        mSettingsStatus = "Settings saved";
        return true;
    }

    void Editor::FEditorUI::SerializeSettings(Serialization::FJsonArchive& Archive) {
        Core::FString Theme{mThemes.GetName(mThemeIndex)};
        float FontSize{mFontSize};
        float Scale{mUIScale.GetUserScale()};
        bool Tearing{mContext->GetRenderer()->IsTearingAllowed()};
        bool PlaceActors{mPlaceActorsPanel.IsVisible()};
        bool Outliner{mOutlinerPanel.IsVisible()};
        bool Details{mDetailsPanel.IsVisible()};
        bool Console{mConsolePanel.IsVisible()};
        bool Statistics{mShowStatistics};
        ImGuiIO& Input{ImGui::GetIO()};
        bool KeyboardNavigation{(Input.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0};
        bool TitleBarOnly{Input.ConfigWindowsMoveFromTitleBarOnly};
        float DoubleClickTime{Input.MouseDoubleClickTime};
        float DragThreshold{Input.MouseDragThreshold};
        float RepeatDelay{Input.KeyRepeatDelay};
        float RepeatRate{Input.KeyRepeatRate};
        Archive.Serialize("Theme", Theme);
        Archive.Serialize("FontSize", FontSize);
        Archive.Serialize("UIScale", Scale);
        Archive.Serialize("AllowTearing", Tearing);
        Archive.Serialize("PlaceActors", PlaceActors);
        Archive.Serialize("Outliner", Outliner);
        Archive.Serialize("Details", Details);
        Archive.Serialize("Console", Console);
        Archive.Serialize("Statistics", Statistics);
        Archive.Serialize("KeyboardNavigation", KeyboardNavigation);
        Archive.Serialize("TitleBarOnly", TitleBarOnly);
        Archive.Serialize("DoubleClickTime", DoubleClickTime);
        Archive.Serialize("DragThreshold", DragThreshold);
        Archive.Serialize("KeyRepeatDelay", RepeatDelay);
        Archive.Serialize("KeyRepeatRate", RepeatRate);

        if (Archive.IsLoading() && !Archive.HasError()) {
            mThemeIndex = mThemes.Find(Theme);
            mFontSize = std::isfinite(FontSize) && FontSize >= 10.0f && FontSize <= 32.0f ? FontSize : 16.0f;
            mUIScale.SetUserScale(Scale);
            mAppearanceDirty = true;
            mContext->GetRenderer()->SetAllowTearing(Tearing);
            mPlaceActorsPanel.SetVisible(PlaceActors);
            mOutlinerPanel.SetVisible(Outliner);
            mDetailsPanel.SetVisible(Details);
            mConsolePanel.SetVisible(Console);
            mShowStatistics = Statistics;
            Input.ConfigFlags = KeyboardNavigation ? Input.ConfigFlags | ImGuiConfigFlags_NavEnableKeyboard : Input.ConfigFlags & ~ImGuiConfigFlags_NavEnableKeyboard;
            Input.ConfigWindowsMoveFromTitleBarOnly = TitleBarOnly;
            Input.MouseDoubleClickTime = std::isfinite(DoubleClickTime) ? std::clamp(DoubleClickTime, 0.1f, 1.0f) : 0.3f;
            Input.MouseDragThreshold = std::isfinite(DragThreshold) ? std::clamp(DragThreshold, 1.0f, 20.0f) : 6.0f;
            Input.KeyRepeatDelay = std::isfinite(RepeatDelay) ? std::clamp(RepeatDelay, 0.1f, 1.0f) : 0.275f;
            Input.KeyRepeatRate = std::isfinite(RepeatRate) ? std::clamp(RepeatRate, 0.02f, 0.3f) : 0.05f;
        }

        for (Core::Uint32 Index{}; Index < 4; ++Index) {
            Editor::FEditorViewport* Viewport{mViewportLayout.GetViewport(Index)};

            if (Viewport != nullptr && Archive.BeginObject("Viewport" + std::to_string(Index))) {
                SerializeViewport(Archive, *Viewport->GetClient(), *Viewport->GetStatistics());
                Archive.EndObject();
            }
        }
    }

    void Editor::FEditorUI::DrawSettingsMenu() {
        if (!ImGui::BeginMenu("Settings")) {
            return;
        }

        if (ImGui::BeginMenu("Theme")) {
            for (Core::Uint32 Index{}; Index < mThemes.GetCount(); ++Index) {
                ImGui::PushID(static_cast<int>(Index));

                if (ImGui::MenuItem(mThemes.GetName(Index).c_str(), nullptr, mThemeIndex == Index)) {
                    mThemeIndex = Index;
                    mAppearanceDirty = true;
                    mSettingsDirty = true;
                }

                ImGui::PopID();
            }

            ImGui::EndMenu();
        }

        mSettingsDirty |= mUIScale.DrawMenu();
        ImGui::BeginDisabled(!mContext->GetRenderer()->IsTearingSupported());
        bool VSync{!mContext->GetRenderer()->IsTearingAllowed()};

        if (ImGui::MenuItem("VSync", nullptr, &VSync)) {
            mSettingsDirty |= mContext->GetRenderer()->SetAllowTearing(!VSync);
        }

        ImGui::EndDisabled();
        ImGui::Separator();

        if (ImGui::MenuItem("Settings...", "Ctrl+,")) {
            mSettingsOpen = true;
            mFocusSettings = true;
        }

        ImGui::EndMenu();
    }

    void Editor::FEditorUI::DrawSettings() {
        if (!mSettingsOpen) {
            return;
        }

        const float Scale{mUIScale.GetScale()};
        const ImGuiViewport* Viewport{ImGui::GetMainViewport()};
        ImGui::SetNextWindowSize({std::min(820.0f * Scale, Viewport->WorkSize.x * 0.9f), std::min(680.0f * Scale, Viewport->WorkSize.y * 0.9f)}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints({std::min(480.0f * Scale, Viewport->WorkSize.x), std::min(340.0f * Scale, Viewport->WorkSize.y)}, {FLT_MAX, FLT_MAX});
        ImGui::SetNextWindowPos(Viewport->GetWorkCenter(), ImGuiCond_Appearing, {0.5f, 0.5f});

        if (mFocusSettings) {
            ImGui::SetNextWindowFocus();
            mFocusSettings = false;
        }

        if (ImGui::Begin("Settings", &mSettingsOpen, ImGuiWindowFlags_NoDocking)) {
            const char* Sections[]{"Interface", "Viewport & Camera", "Gizmo & Snapping", "Rendering", "Panels & Statistics", "Project"};
            const float FooterHeight{ImGui::GetFrameHeightWithSpacing() * 2.0f};

            if (ImGui::BeginChild("SettingsNavigation", {170.0f * Scale, -FooterHeight}, ImGuiChildFlags_Borders)) {
                for (int Index{}; Index < static_cast<int>(std::size(Sections)); ++Index) {
                    if (ImGui::Selectable(Sections[Index], mSettingsSection == Index)) {
                        mSettingsSection = Index;
                    }
                }
            }

            ImGui::EndChild();
            ImGui::SameLine();

            if (ImGui::BeginChild("SettingsValues", {0.0f, -FooterHeight}, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar)) {
                ImGui::SeparatorText(Sections[mSettingsSection]);
                ImGui::PushItemWidth(std::max(140.0f * Scale, ImGui::GetContentRegionAvail().x * 0.55f));

                if (mSettingsSection == 0) {
                    DrawInterfaceSettings();
                } else if (mSettingsSection == 1 || mSettingsSection == 2 || mSettingsSection == 4) {
                    const std::array<const char*, 4> Names{"Viewport 1", "Viewport 2", "Viewport 3", "Viewport 4"};
                    int Selected{static_cast<int>(mSettingsViewport)};

                    if (ImGui::Combo("Viewport", &Selected, Names.data(), static_cast<int>(Names.size()))) {
                        mSettingsViewport = static_cast<Core::Uint32>(Selected);
                    }

                    ImGui::TextDisabled("%s", mViewportLayout.GetViewport(mSettingsViewport)->GetClient()->GetViewTypeName());
                    ImGui::Spacing();

                    if (mSettingsSection == 1) {
                        DrawViewportSettings();
                    } else if (mSettingsSection == 2) {
                        DrawGizmoSettings();
                    } else {
                        DrawPanelSettings();
                    }
                } else if (mSettingsSection == 3) {
                    DrawRenderingSettings();
                } else {
                    DrawProjectSettings();
                }

                ImGui::PopItemWidth();
            }

            ImGui::EndChild();
            ImGui::Separator();

            if (ImGui::Button("Save Settings")) {
                SaveSettings();
            }

            ImGui::SameLine();

            if (ImGui::Button("Reset to Defaults")) {
                ImGui::OpenPopup("Reset Settings?");
            }

            ImGui::SameLine();

            if (ImGui::Button("Close")) {
                mSettingsOpen = false;
            }

            if (ImGui::BeginPopupModal("Reset Settings?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextUnformatted("Reset editor preferences for this project?");
                ImGui::BeginDisabled(!mViewportLayout.CanChangeLayout());

                if (ImGui::Button("Reset")) {
                    ResetSettings();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndDisabled();
                ImGui::SameLine();

                if (ImGui::Button("Cancel")) {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            ImGui::TextDisabled("%s", !mSettingsStatus.empty() ? mSettingsStatus.c_str() : mSettingsDirty ? "Changes apply immediately and save automatically." : "Preferences are saved per project.");
        }

        ImGui::End();

        if (!mSettingsOpen && mSettingsDirty) {
            SaveSettings();
        }
    }

    void Editor::FEditorUI::DrawInterfaceSettings() {
        ImGui::TextDisabled("ImThemes: %u themes", mThemes.GetCount() - 1);
        ImGui::InputTextWithHint("Search", "Filter themes...", mThemeFilter.data(), mThemeFilter.size());

        if (ImGui::BeginCombo("Theme", mThemes.GetName(mThemeIndex).c_str())) {
            ImGuiTextFilter Filter{mThemeFilter.data()};

            for (Core::Uint32 Index{}; Index < mThemes.GetCount(); ++Index) {
                if (!Filter.PassFilter(mThemes.GetName(Index).c_str()) && !Filter.PassFilter(mThemes.GetAuthor(Index).c_str())) {
                    continue;
                }

                ImGui::PushID(static_cast<int>(Index));

                if (ImGui::Selectable(mThemes.GetName(Index).c_str(), mThemeIndex == Index)) {
                    mThemeIndex = Index;
                    mAppearanceDirty = true;
                    mSettingsDirty = true;
                }

                if (mThemeIndex == Index) {
                    ImGui::SetItemDefaultFocus();
                }

                ImGui::PopID();
            }

            ImGui::EndCombo();
        }

        ImGui::TextWrapped("%s", mThemes.GetDescription(mThemeIndex).c_str());
        ImGui::TextDisabled("By %s", mThemes.GetAuthor(mThemeIndex).c_str());

        if (ImGui::Button("Import ImThemes TOML...")) {
            ImportTheme();
        }

        ImGui::SeparatorText("Size");
        bool Automatic{mUIScale.GetUserScale() == 0.0f};

        if (ImGui::Checkbox("Automatic UI Scale", &Automatic)) {
            mSettingsDirty |= mUIScale.SetUserScale(Automatic ? 0.0f : 1.0f);
        }

        ImGui::BeginDisabled(Automatic);
        float Scale{Automatic ? 1.0f : mUIScale.GetUserScale()};

        if (ImGui::SliderFloat("UI Scale", &Scale, 0.75f, 3.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp)) {
            mSettingsDirty |= mUIScale.SetUserScale(Scale);
        }

        ImGui::EndDisabled();
        ImGui::TextDisabled("Effective scale: %.2fx (includes display DPI)", mUIScale.GetScale());

        if (ImGui::SliderFloat("Font Size", &mFontSize, 10.0f, 32.0f, "%.0f px", ImGuiSliderFlags_AlwaysClamp)) {
            mAppearanceDirty = true;
            mSettingsDirty = true;
        }

        ImGui::SeparatorText("Input & Windows");
        ImGuiIO& Input{ImGui::GetIO()};
        bool Navigation{(Input.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0};

        if (ImGui::Checkbox("Keyboard Navigation", &Navigation)) {
            Input.ConfigFlags = Navigation ? Input.ConfigFlags | ImGuiConfigFlags_NavEnableKeyboard : Input.ConfigFlags & ~ImGuiConfigFlags_NavEnableKeyboard;
            mSettingsDirty = true;
        }

        mSettingsDirty |= ImGui::Checkbox("Move Windows from Title Bar Only", &Input.ConfigWindowsMoveFromTitleBarOnly);
        mSettingsDirty |= ImGui::SliderFloat("Double Click Time", &Input.MouseDoubleClickTime, 0.1f, 1.0f, "%.2f s", ImGuiSliderFlags_AlwaysClamp);
        mSettingsDirty |= ImGui::SliderFloat("Mouse Drag Threshold", &Input.MouseDragThreshold, 1.0f, 20.0f, "%.0f px", ImGuiSliderFlags_AlwaysClamp);
        mSettingsDirty |= ImGui::SliderFloat("Key Repeat Delay", &Input.KeyRepeatDelay, 0.1f, 1.0f, "%.2f s", ImGuiSliderFlags_AlwaysClamp);
        mSettingsDirty |= ImGui::SliderFloat("Key Repeat Rate", &Input.KeyRepeatRate, 0.02f, 0.3f, "%.2f s", ImGuiSliderFlags_AlwaysClamp);
    }

    void Editor::FEditorUI::DrawViewportSettings() {
        Editor::FEditorViewportClient& Client{*mViewportLayout.GetViewport(mSettingsViewport)->GetClient()};
        ImGui::BeginDisabled(!mViewportLayout.CanChangeLayout());
        bool QuadView{mViewportLayout.IsQuadView()};

        if (ImGui::Checkbox("Four Viewports", &QuadView)) {
            mViewportLayout.SetQuadView(QuadView);
            mSettingsDirty = true;
        }

        float Horizontal{mViewportLayout.GetHorizontalRatio()};
        float Vertical{mViewportLayout.GetVerticalRatio()};

        bool SplitChanged{ImGui::SliderFloat("Horizontal Split", &Horizontal, 0.1f, 0.9f, "%.2f", ImGuiSliderFlags_AlwaysClamp)};
        SplitChanged |= ImGui::SliderFloat("Vertical Split", &Vertical, 0.1f, 0.9f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

        if (SplitChanged) {
            mSettingsDirty |= mViewportLayout.SetSplitRatios(Horizontal, Vertical);
        }

        ImGui::SeparatorText("Camera");
        mSettingsDirty |= DrawClientFloat("Movement Speed", Client, &Editor::FEditorViewportClient::GetCameraSpeed, &Editor::FEditorViewportClient::SetCameraSpeed, 0.1f, 128.0f, "%.2f");
        mSettingsDirty |= DrawClientFloat("Mouse Sensitivity", Client, &Editor::FEditorViewportClient::GetCameraSensitivity, &Editor::FEditorViewportClient::SetCameraSensitivity, 0.01f, 2.0f, "%.3f");
        mSettingsDirty |= DrawClientFloat("Orthographic Width", Client, &Editor::FEditorViewportClient::GetOrthographicWidth, &Editor::FEditorViewportClient::SetOrthographicWidth, 0.01f, 100000.0f, "%.2f");

        Engine::FEditorCameraState Camera{Client.GetPerspectiveCamera()};
        float FieldOfView{Camera.GetFieldOfView()};
        float NearClip{Camera.GetNearClip()};
        float FarClip{Camera.GetFarClip()};
        bool ProjectionChanged{ImGui::SliderFloat("Field of View", &FieldOfView, 1.0f, 179.0f, "%.1f degrees", ImGuiSliderFlags_AlwaysClamp)};
        ProjectionChanged |= ImGui::DragFloat("Near Clip", &NearClip, 0.01f, 0.001f, FarClip - 0.001f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        ProjectionChanged |= ImGui::DragFloat("Far Clip", &FarClip, 10.0f, NearClip + 0.001f, 1000000.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);

        if (ProjectionChanged && Camera.Set(Camera.GetLocation(), Camera.GetRotation(), FieldOfView, NearClip, FarClip)) {
            Client.SetPerspectiveCamera(Camera);
        }

        ImGui::TextWrapped("Camera projection is stored with the scene when you save it.");
        ImGui::SeparatorText("Display");
        const auto Current{std::find(ViewModes.begin(), ViewModes.end(), Client.GetColorViewMode())};
        int Mode{Current == ViewModes.end() ? 2 : static_cast<int>(Current - ViewModes.begin())};

        if (ImGui::Combo("View Mode", &Mode, ViewModeNames.data(), static_cast<int>(ViewModeNames.size()))) {
            Client.SetViewMode(ViewModes[Mode]);
            mSettingsDirty = true;
        }

        const std::array<const char*, 3> BufferNames{"Scene Color", "World Normal", "Scene Depth"};
        const std::array<Engine::EViewMode, 3> BufferModes{Client.GetColorViewMode(), Engine::EViewMode::WorldNormal, Engine::EViewMode::SceneDepth};
        int Buffer{Client.GetViewMode() == Engine::EViewMode::WorldNormal ? 1 : Client.GetViewMode() == Engine::EViewMode::SceneDepth ? 2 : 0};

        if (ImGui::Combo("Buffer Visualization", &Buffer, BufferNames.data(), static_cast<int>(BufferNames.size()))) {
            Client.SetViewMode(BufferModes[Buffer]);
            mSettingsDirty = true;
        }

        mSettingsDirty |= DrawClientFloat("Grid Spacing", Client, &Editor::FEditorViewportClient::GetGridSpacing, &Editor::FEditorViewportClient::SetGridSpacing, 0.01f, 1000.0f, "%.2f");
        mSettingsDirty |= DrawClientFloat("Line Thickness", Client, &Editor::FEditorViewportClient::GetLineThickness, &Editor::FEditorViewportClient::SetLineThickness, 0.5f, 10.0f, "%.1f px");
        Engine::FEngineShowFlags Flags{Client.GetShowFlags()};
        bool Changed{};

        for (const auto& [Name, Member] : ShowOptions) {
            Changed |= ImGui::Checkbox(Name, &(Flags.*Member));
        }

        if (Changed) {
            Client.SetShowFlags(Flags);
            mSettingsDirty = true;
        }

        ImGui::EndDisabled();
    }

    void Editor::FEditorUI::DrawGizmoSettings() {
        Editor::FEditorViewportClient& Client{*mViewportLayout.GetViewport(mSettingsViewport)->GetClient()};
        ImGui::BeginDisabled(!Client.CanEditGizmoSettings());
        const char* Modes[]{"Select", "Translate", "Rotate", "Scale"};
        const Editor::EViewportCommand Commands[]{Editor::EViewportCommand::Select, Editor::EViewportCommand::Translate, Editor::EViewportCommand::Rotate, Editor::EViewportCommand::Scale};
        int Mode{Client.GetGizmoMode() == Editor::EGizmoMode::None ? 0 : static_cast<int>(Client.GetGizmoMode()) + 1};

        if (ImGui::Combo("Transform Mode", &Mode, Modes, static_cast<int>(std::size(Modes)))) {
            mSettingsDirty |= Client.ExecuteCommand(Commands[Mode]);
        }

        ImGui::BeginDisabled(!Client.CanExecuteCommand(Editor::EViewportCommand::WorldSpace));
        const char* Spaces[]{"World", "Local"};
        int Space{Client.GetGizmoSpace() == Editor::EGizmoSpace::World ? 0 : 1};

        if (ImGui::Combo("Coordinate System", &Space, Spaces, static_cast<int>(std::size(Spaces)))) {
            mSettingsDirty |= Client.ExecuteCommand(Space == 0 ? Editor::EViewportCommand::WorldSpace : Editor::EViewportCommand::LocalSpace);
        }

        ImGui::EndDisabled();
        ImGui::SeparatorText("Snapping");

        for (const auto& [Label, Command] : {std::pair{"Translation Snap", Editor::EViewportCommand::ToggleTranslationSnap}, std::pair{"Rotation Snap", Editor::EViewportCommand::ToggleRotationSnap}, std::pair{"Scale Snap", Editor::EViewportCommand::ToggleScaleSnap}}) {
            bool Enabled{Client.IsCommandChecked(Command)};

            if (ImGui::Checkbox(Label, &Enabled)) {
                mSettingsDirty |= Client.ExecuteCommand(Command);
            }
        }

        const Editor::FGizmoSettings Settings{*Client.GetGizmoSettings()};
        float Translation{Settings.mTranslationStep};
        float Rotation{Settings.mRotationStep};
        float Scale{Settings.mScaleStep};
        float Size{Settings.mSize};

        if (ImGui::DragFloat("Translation Step", &Translation, 0.01f, 0.001f, 10000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
            mSettingsDirty |= Client.SetGizmoSetting(Editor::EGizmoSetting::TranslationStep, Translation);
        }

        if (ImGui::DragFloat("Rotation Step", &Rotation, 0.1f, 0.001f, 360.0f, "%.3f degrees", ImGuiSliderFlags_AlwaysClamp)) {
            mSettingsDirty |= Client.SetGizmoSetting(Editor::EGizmoSetting::RotationStep, Rotation);
        }

        if (ImGui::DragFloat("Scale Step", &Scale, 0.01f, 0.001f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
            mSettingsDirty |= Client.SetGizmoSetting(Editor::EGizmoSetting::ScaleStep, Scale);
        }

        if (ImGui::SliderFloat("Gizmo Size", &Size, 0.5f, 2.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp)) {
            mSettingsDirty |= Client.SetGizmoSetting(Editor::EGizmoSetting::Size, Size);
        }

        ImGui::EndDisabled();
    }

    void Editor::FEditorUI::DrawRenderingSettings() {
        RenderCore::IRendererModule& Renderer{*mContext->GetRenderer()};
        bool VSync{!Renderer.IsTearingAllowed()};
        ImGui::BeginDisabled(!Renderer.IsTearingSupported());

        if (ImGui::Checkbox("VSync", &VSync)) {
            mSettingsDirty |= Renderer.SetAllowTearing(!VSync);
        }

        ImGui::EndDisabled();
        ImGui::TextWrapped("%s", Renderer.IsTearingSupported() ? "Disabling VSync allows tearing in windowed mode." : "Tearing is unavailable on this device. VSync is enabled.");

        if (ImGui::Button("Reload Changed Shaders")) {
            mSettingsStatus = Renderer.ReloadChangedShaders() ? "Shaders reloaded" : "Shader reload failed; see Console";
        }

        if (ImGui::Button("Open Shadow Atlas")) {
            mContext->SetShadowAtlasOpen(true);
        }

        ImGui::TextWrapped("View mode, shadows and visibility filters are available under Viewport & Camera for each viewport.");
    }

    void Editor::FEditorUI::DrawPanelSettings() {
        for (Editor::FEditorPanel* Panel : {static_cast<Editor::FEditorPanel*>(&mPlaceActorsPanel), static_cast<Editor::FEditorPanel*>(&mOutlinerPanel), static_cast<Editor::FEditorPanel*>(&mDetailsPanel), static_cast<Editor::FEditorPanel*>(&mConsolePanel)}) {
            bool Visible{Panel->IsVisible()};

            if (ImGui::Checkbox(Panel->GetName().c_str(), &Visible)) {
                Panel->SetVisible(Visible);
                mSettingsDirty = true;
            }
        }

        mSettingsDirty |= ImGui::Checkbox("Statistics Window", &mShowStatistics);
        ImGui::SeparatorText("Viewport Statistics Overlay");
        Editor::FViewportStatistics& Statistics{*mViewportLayout.GetViewport(mSettingsViewport)->GetStatistics()};

        for (const Core::FString& Group : Core::FStats::Get().GetGroups()) {
            bool Enabled{Statistics.IsEnabled(Group)};

            if (ImGui::Checkbox(Group.c_str(), &Enabled)) {
                Core::FString Output{};
                mSettingsDirty |= Statistics.ExecuteCommand("stat " + Group + (Enabled ? " on" : " off"), Output);
            }
        }
    }

    void Editor::FEditorUI::DrawProjectSettings() {
        Engine::FProject& Project{*mContext->GetProject()};
        ImGui::TextWrapped("Project: %s", Project.GetName().c_str());
        ImGui::TextWrapped("Location: %s", Core::FilePath::ToUtf8(Project.GetFilePath()).c_str());
        ImGui::SeparatorText("Startup Scene");
        ImGui::BeginDisabled(!mContext->CanEditWorld());
        ImGui::InputText("Content Path", mStartupScene.data(), mStartupScene.size());

        if (ImGui::Button("Save Startup Scene")) {
            Core::FString Error{};
            mSettingsStatus = Project.SetStartupScene(Core::FilePath::FromUtf8(mStartupScene.data()), Error) ? "Startup scene saved" : Error;
        }

        ImGui::EndDisabled();
        ImGui::SeparatorText("Preferences");
        ImGui::TextWrapped("Settings: %s", Core::FilePath::ToUtf8(mSettingsPath).c_str());
        ImGui::TextWrapped("Custom themes: %s", Core::FilePath::ToUtf8(mSettingsPath.parent_path() / "Themes").c_str());
    }

    void Editor::FEditorUI::ImportTheme() {
        std::array<wchar_t, 32768> Filename{};
        OPENFILENAMEW Dialog{};
        Dialog.lStructSize = sizeof(Dialog);
        Dialog.hwndOwner = mPlatform->GetWindowHandle();
        Dialog.lpstrFilter = L"ImThemes TOML\0*.toml\0\0";
        Dialog.lpstrFile = Filename.data();
        Dialog.nMaxFile = static_cast<DWORD>(Filename.size());
        Dialog.lpstrTitle = L"Import ImThemes theme";
        Dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

        if (!GetOpenFileNameW(&Dialog)) {
            return;
        }

        const std::filesystem::path Source{Filename.data()};
        Editor::FImGuiThemeCatalog Candidate{};
        Core::FString Error{};

        if (!Candidate.Load(Source, Error)) {
            mSettingsStatus = Error;
            return;
        }

        const std::filesystem::path Target{mSettingsPath.parent_path() / "Themes" / Source.filename()};
        std::error_code Code{};
        std::filesystem::create_directories(Target.parent_path(), Code);

        const std::filesystem::path CanonicalSource{Code ? std::filesystem::path{} : std::filesystem::weakly_canonical(Source, Code)};
        const std::filesystem::path CanonicalTarget{Code ? std::filesystem::path{} : std::filesystem::weakly_canonical(Target, Code)};

        if (!Code && CanonicalSource != CanonicalTarget) {
            std::filesystem::copy_file(Source, Target, std::filesystem::copy_options::overwrite_existing, Code);
        }

        if (Code || !mThemes.Load(Target, Error)) {
            mSettingsStatus = Code ? "Cannot save imported theme: " + Code.message() : Error;
            return;
        }

        mThemeIndex = mThemes.Find(Candidate.GetName(1));
        mAppearanceDirty = true;
        mSettingsDirty = true;
        mSettingsStatus = "Theme imported";
    }

    void Editor::FEditorUI::ResetSettings() {
        mThemeIndex = 0;
        mFontSize = 16.0f;
        mUIScale.SetUserScale(0.0f);
        mAppearanceDirty = true;
        mContext->GetRenderer()->SetAllowTearing(false);
        mPlaceActorsPanel.SetVisible(true);
        mOutlinerPanel.SetVisible(true);
        mDetailsPanel.SetVisible(true);
        mConsolePanel.SetVisible(true);
        mShowStatistics = false;
        ImGuiIO& Input{ImGui::GetIO()};
        Input.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        Input.ConfigWindowsMoveFromTitleBarOnly = false;
        Input.MouseDoubleClickTime = 0.3f;
        Input.MouseDragThreshold = 6.0f;
        Input.KeyRepeatDelay = 0.275f;
        Input.KeyRepeatRate = 0.05f;
        mViewportLayout.SetQuadView(false);
        mViewportLayout.SetSplitRatios(0.5f, 0.5f);

        for (Core::Uint32 Index{}; Index < 4; ++Index) {
            Editor::FEditorViewport* Viewport{mViewportLayout.GetViewport(Index)};
            Editor::FEditorViewportClient& Client{*Viewport->GetClient()};
            Client.SetCameraSpeed(4.0f);
            Client.SetCameraSensitivity(0.2f);
            Client.SetGridSpacing(1.0f);
            Client.SetLineThickness(1.5f);
            Client.SetOrthographicWidth(20.0f);
            Client.SetViewMode(Engine::EViewMode::Lit_Phong);
            Client.SetShowFlags(Engine::FEngineShowFlags{});
            Client.SetGizmoSetting(Editor::EGizmoSetting::TranslationStep, 1.0f);
            Client.SetGizmoSetting(Editor::EGizmoSetting::RotationStep, 15.0f);
            Client.SetGizmoSetting(Editor::EGizmoSetting::ScaleStep, 0.1f);
            Client.SetGizmoSetting(Editor::EGizmoSetting::Size, 1.0f);

            for (Editor::EViewportCommand Command : {Editor::EViewportCommand::ToggleTranslationSnap, Editor::EViewportCommand::ToggleRotationSnap, Editor::EViewportCommand::ToggleScaleSnap}) {
                if (Client.IsCommandChecked(Command)) {
                    Client.ExecuteCommand(Command);
                }
            }

            Client.ExecuteCommand(Editor::EViewportCommand::Translate);
            Client.ExecuteCommand(Editor::EViewportCommand::WorldSpace);
            Core::FString Output{};
            Viewport->GetStatistics()->ExecuteCommand("stat none", Output);
        }

        mSettingsDirty = true;
    }

}
