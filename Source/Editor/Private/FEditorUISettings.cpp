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

namespace {
    const std::array<std::pair<const char*, bool FEngineShowFlags::*>, 9> ShowOptions{{
        {"Primitives", &FEngineShowFlags::mPrimitives},
        {"Sprites", &FEngineShowFlags::mSprites},
        {"Text", &FEngineShowFlags::mText},
        {"Shadows", &FEngineShowFlags::mShadows},
        {"Light Icons", &FEngineShowFlags::mLightIcons},
        {"Light Visualizers", &FEngineShowFlags::mLightVisualizers},
        {"Bounds", &FEngineShowFlags::mBounds},
        {"Grid", &FEngineShowFlags::mGrid},
        {"Axes", &FEngineShowFlags::mAxes}
    }};

    const std::array<EViewMode, 6> ViewModes{EViewMode::Lit_Gouraud, EViewMode::Lit_Lambert, EViewMode::Lit_Phong, EViewMode::Unlit, EViewMode::WorldNormal, EViewMode::Wireframe};
    const std::array<const char*, 6> ViewModeNames{"Lit - Gouraud", "Lit - Lambert", "Lit - Phong", "Unlit", "World Normal", "Wireframe"};

    bool DrawClientFloat(const char* Label, FEditorViewportClient& Client, float (FEditorViewportClient::*Getter)() const, bool (FEditorViewportClient::*Setter)(float), float Minimum, float Maximum, const char* Format) {
        float Value{(Client.*Getter)()};
        return ImGui::DragFloat(Label, &Value, Minimum * 0.1f, Minimum, Maximum, Format, ImGuiSliderFlags_AlwaysClamp) && (Client.*Setter)(Value);
    }

    void SerializeViewport(FJsonArchive& Archive, FEditorViewportClient& Client, FViewportStatistics& Statistics) {
        float Speed{Client.GetCameraSpeed()};
        float Sensitivity{Client.GetCameraSensitivity()};
        float Spacing{Client.GetGridSpacing()};
        float Thickness{Client.GetLineThickness()};
        float Width{Client.GetOrthographicWidth()};
        Int32 Mode{static_cast<Int32>(Client.GetViewMode())};
        FGizmoSettings Gizmo{*Client.GetGizmoSettings()};
        Int32 TransformMode{static_cast<Int32>(Client.GetGizmoMode())};
        Int32 Space{static_cast<Int32>(Client.GetGizmoSpace())};
        FEngineShowFlags Flags{Client.GetShowFlags()};
        Archive.Serialize("CameraSpeed", Speed);
        Archive.Serialize("CameraSensitivity", Sensitivity);
        Archive.Serialize("GridSpacing", Spacing);
        Archive.Serialize("LineThickness", Thickness);
        Archive.Serialize("OrthographicWidth", Width);
        Archive.Serialize("ViewMode", Mode);
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
            Archive.Serialize(Name, Flags.*Member);
        }

        TArray<FString> Groups{FStats::Get().GetGroups()};
        FString EnabledGroups{"\n"};

        for (const FString& Group : Groups) {
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
        Client.SetViewMode(static_cast<EViewMode>(Mode));
        Client.SetShowFlags(Flags);
        Client.SetGizmoSetting(EGizmoSetting::TranslationStep, Gizmo.mTranslationStep);
        Client.SetGizmoSetting(EGizmoSetting::RotationStep, Gizmo.mRotationStep);
        Client.SetGizmoSetting(EGizmoSetting::ScaleStep, Gizmo.mScaleStep);
        Client.SetGizmoSetting(EGizmoSetting::Size, Gizmo.mSize);

        for (const auto& [Command, Enabled] : {std::pair{EViewportCommand::ToggleTranslationSnap, Gizmo.mTranslationSnap}, std::pair{EViewportCommand::ToggleRotationSnap, Gizmo.mRotationSnap}, std::pair{EViewportCommand::ToggleScaleSnap, Gizmo.mScaleSnap}}) {
            if (Client.IsCommandChecked(Command) != Enabled) {
                Client.ExecuteCommand(Command);
            }
        }

        const std::array<EViewportCommand, 4> Commands{EViewportCommand::Translate, EViewportCommand::Rotate, EViewportCommand::Scale, EViewportCommand::Select};

        if (TransformMode >= 0 && static_cast<size_t>(TransformMode) < Commands.size()) {
            Client.ExecuteCommand(Commands[TransformMode]);
        }

        if (Space == static_cast<Int32>(EGizmoSpace::World) || Space == static_cast<Int32>(EGizmoSpace::Local)) {
            Client.ExecuteCommand(Space == static_cast<Int32>(EGizmoSpace::World) ? EViewportCommand::WorldSpace : EViewportCommand::LocalSpace);
        }

        FString Output{};
        Statistics.ExecuteCommand("stat none", Output);

        for (const FString& Group : Groups) {
            if (EnabledGroups.find('\n' + Group + '\n') != FString::npos) {
                Statistics.ExecuteCommand("stat " + Group + " on", Output);
            }
        }
    }
}

void FEditorUI::InitializeSettings() {
    mThemes = FImGuiThemeCatalog{};
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

    FString Error{};

    if (!mThemes.Load(Bundled, Error)) {
        mContext->AddLog(Error);
        mSettingsStatus = Error;
    }

    const std::filesystem::path Imported{mSettingsPath.parent_path() / "Themes"};
    TArray<std::filesystem::path> Files{};

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

void FEditorUI::LoadSettings() {
    std::error_code Code{};

    if (!std::filesystem::is_regular_file(mSettingsPath, Code)) {
        return;
    }

    FJsonArchive Archive{EArchiveMode::Loading};

    if (!Archive.Load(mSettingsPath)) {
        mSettingsStatus = Archive.GetError();
        mContext->AddLog(mSettingsStatus);
        return;
    }

    Uint32 Version{};
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

bool FEditorUI::SaveSettings() {
    if (mSettingsPath.empty()) {
        return false;
    }

    FJsonArchive Archive{EArchiveMode::Saving};
    Uint32 Version{1};
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

void FEditorUI::SerializeSettings(FJsonArchive& Archive) {
    FString Theme{mThemes.GetName(mThemeIndex)};
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

    for (Uint32 Index{}; Index < 4; ++Index) {
        FEditorViewport* Viewport{mViewportLayout.GetViewport(Index)};

        if (Viewport != nullptr && Archive.BeginObject("Viewport" + std::to_string(Index))) {
            SerializeViewport(Archive, *Viewport->GetClient(), *Viewport->GetStatistics());
            Archive.EndObject();
        }
    }
}

void FEditorUI::DrawSettingsMenu() {
    if (!ImGui::BeginMenu("Settings")) {
        return;
    }

    if (ImGui::BeginMenu("Theme")) {
        for (Uint32 Index{}; Index < mThemes.GetCount(); ++Index) {
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

void FEditorUI::DrawSettings() {
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
                    mSettingsViewport = static_cast<Uint32>(Selected);
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

void FEditorUI::DrawInterfaceSettings() {
    ImGui::TextDisabled("ImThemes: %u themes", mThemes.GetCount() - 1);
    ImGui::InputTextWithHint("Search", "Filter themes...", mThemeFilter.data(), mThemeFilter.size());

    if (ImGui::BeginCombo("Theme", mThemes.GetName(mThemeIndex).c_str())) {
        ImGuiTextFilter Filter{mThemeFilter.data()};

        for (Uint32 Index{}; Index < mThemes.GetCount(); ++Index) {
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

void FEditorUI::DrawViewportSettings() {
    FEditorViewportClient& Client{*mViewportLayout.GetViewport(mSettingsViewport)->GetClient()};
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
    mSettingsDirty |= DrawClientFloat("Movement Speed", Client, &FEditorViewportClient::GetCameraSpeed, &FEditorViewportClient::SetCameraSpeed, 0.1f, 128.0f, "%.2f");
    mSettingsDirty |= DrawClientFloat("Mouse Sensitivity", Client, &FEditorViewportClient::GetCameraSensitivity, &FEditorViewportClient::SetCameraSensitivity, 0.01f, 2.0f, "%.3f");
    mSettingsDirty |= DrawClientFloat("Orthographic Width", Client, &FEditorViewportClient::GetOrthographicWidth, &FEditorViewportClient::SetOrthographicWidth, 0.01f, 100000.0f, "%.2f");

    FEditorCameraState Camera{Client.GetPerspectiveCamera()};
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
    const auto Current{std::find(ViewModes.begin(), ViewModes.end(), Client.GetViewMode())};
    int Mode{Current == ViewModes.end() ? 2 : static_cast<int>(Current - ViewModes.begin())};

    if (ImGui::Combo("View Mode", &Mode, ViewModeNames.data(), static_cast<int>(ViewModeNames.size()))) {
        Client.SetViewMode(ViewModes[Mode]);
        mSettingsDirty = true;
    }

    mSettingsDirty |= DrawClientFloat("Grid Spacing", Client, &FEditorViewportClient::GetGridSpacing, &FEditorViewportClient::SetGridSpacing, 0.01f, 1000.0f, "%.2f");
    mSettingsDirty |= DrawClientFloat("Line Thickness", Client, &FEditorViewportClient::GetLineThickness, &FEditorViewportClient::SetLineThickness, 0.5f, 10.0f, "%.1f px");
    FEngineShowFlags Flags{Client.GetShowFlags()};
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

void FEditorUI::DrawGizmoSettings() {
    FEditorViewportClient& Client{*mViewportLayout.GetViewport(mSettingsViewport)->GetClient()};
    ImGui::BeginDisabled(!Client.CanEditGizmoSettings());
    const char* Modes[]{"Select", "Translate", "Rotate", "Scale"};
    const EViewportCommand Commands[]{EViewportCommand::Select, EViewportCommand::Translate, EViewportCommand::Rotate, EViewportCommand::Scale};
    int Mode{Client.GetGizmoMode() == EGizmoMode::None ? 0 : static_cast<int>(Client.GetGizmoMode()) + 1};

    if (ImGui::Combo("Transform Mode", &Mode, Modes, static_cast<int>(std::size(Modes)))) {
        mSettingsDirty |= Client.ExecuteCommand(Commands[Mode]);
    }

    ImGui::BeginDisabled(!Client.CanExecuteCommand(EViewportCommand::WorldSpace));
    const char* Spaces[]{"World", "Local"};
    int Space{Client.GetGizmoSpace() == EGizmoSpace::World ? 0 : 1};

    if (ImGui::Combo("Coordinate System", &Space, Spaces, static_cast<int>(std::size(Spaces)))) {
        mSettingsDirty |= Client.ExecuteCommand(Space == 0 ? EViewportCommand::WorldSpace : EViewportCommand::LocalSpace);
    }

    ImGui::EndDisabled();
    ImGui::SeparatorText("Snapping");

    for (const auto& [Label, Command] : {std::pair{"Translation Snap", EViewportCommand::ToggleTranslationSnap}, std::pair{"Rotation Snap", EViewportCommand::ToggleRotationSnap}, std::pair{"Scale Snap", EViewportCommand::ToggleScaleSnap}}) {
        bool Enabled{Client.IsCommandChecked(Command)};

        if (ImGui::Checkbox(Label, &Enabled)) {
            mSettingsDirty |= Client.ExecuteCommand(Command);
        }
    }

    const FGizmoSettings Settings{*Client.GetGizmoSettings()};
    float Translation{Settings.mTranslationStep};
    float Rotation{Settings.mRotationStep};
    float Scale{Settings.mScaleStep};
    float Size{Settings.mSize};

    if (ImGui::DragFloat("Translation Step", &Translation, 0.01f, 0.001f, 10000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
        mSettingsDirty |= Client.SetGizmoSetting(EGizmoSetting::TranslationStep, Translation);
    }

    if (ImGui::DragFloat("Rotation Step", &Rotation, 0.1f, 0.001f, 360.0f, "%.3f degrees", ImGuiSliderFlags_AlwaysClamp)) {
        mSettingsDirty |= Client.SetGizmoSetting(EGizmoSetting::RotationStep, Rotation);
    }

    if (ImGui::DragFloat("Scale Step", &Scale, 0.01f, 0.001f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
        mSettingsDirty |= Client.SetGizmoSetting(EGizmoSetting::ScaleStep, Scale);
    }

    if (ImGui::SliderFloat("Gizmo Size", &Size, 0.5f, 2.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp)) {
        mSettingsDirty |= Client.SetGizmoSetting(EGizmoSetting::Size, Size);
    }

    ImGui::EndDisabled();
}

void FEditorUI::DrawRenderingSettings() {
    IRendererModule& Renderer{*mContext->GetRenderer()};
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

void FEditorUI::DrawPanelSettings() {
    for (FEditorPanel* Panel : {static_cast<FEditorPanel*>(&mPlaceActorsPanel), static_cast<FEditorPanel*>(&mOutlinerPanel), static_cast<FEditorPanel*>(&mDetailsPanel), static_cast<FEditorPanel*>(&mConsolePanel)}) {
        bool Visible{Panel->IsVisible()};

        if (ImGui::Checkbox(Panel->GetName().c_str(), &Visible)) {
            Panel->SetVisible(Visible);
            mSettingsDirty = true;
        }
    }

    mSettingsDirty |= ImGui::Checkbox("Statistics Window", &mShowStatistics);
    ImGui::SeparatorText("Viewport Statistics Overlay");
    FViewportStatistics& Statistics{*mViewportLayout.GetViewport(mSettingsViewport)->GetStatistics()};

    for (const FString& Group : FStats::Get().GetGroups()) {
        bool Enabled{Statistics.IsEnabled(Group)};

        if (ImGui::Checkbox(Group.c_str(), &Enabled)) {
            FString Output{};
            mSettingsDirty |= Statistics.ExecuteCommand("stat " + Group + (Enabled ? " on" : " off"), Output);
        }
    }
}

void FEditorUI::DrawProjectSettings() {
    FProject& Project{*mContext->GetProject()};
    ImGui::TextWrapped("Project: %s", Project.GetName().c_str());
    ImGui::TextWrapped("Location: %s", FilePath::ToUtf8(Project.GetFilePath()).c_str());
    ImGui::SeparatorText("Startup Scene");
    ImGui::BeginDisabled(!mContext->CanEditWorld());
    ImGui::InputText("Content Path", mStartupScene.data(), mStartupScene.size());

    if (ImGui::Button("Save Startup Scene")) {
        FString Error{};
        mSettingsStatus = Project.SetStartupScene(FilePath::FromUtf8(mStartupScene.data()), Error) ? "Startup scene saved" : Error;
    }

    ImGui::EndDisabled();
    ImGui::SeparatorText("Preferences");
    ImGui::TextWrapped("Settings: %s", FilePath::ToUtf8(mSettingsPath).c_str());
    ImGui::TextWrapped("Custom themes: %s", FilePath::ToUtf8(mSettingsPath.parent_path() / "Themes").c_str());
}

void FEditorUI::ImportTheme() {
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
    FImGuiThemeCatalog Candidate{};
    FString Error{};

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

void FEditorUI::ResetSettings() {
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

    for (Uint32 Index{}; Index < 4; ++Index) {
        FEditorViewport* Viewport{mViewportLayout.GetViewport(Index)};
        FEditorViewportClient& Client{*Viewport->GetClient()};
        Client.SetCameraSpeed(4.0f);
        Client.SetCameraSensitivity(0.2f);
        Client.SetGridSpacing(1.0f);
        Client.SetLineThickness(1.5f);
        Client.SetOrthographicWidth(20.0f);
        Client.SetViewMode(EViewMode::Lit_Phong);
        Client.SetShowFlags(FEngineShowFlags{});
        Client.SetGizmoSetting(EGizmoSetting::TranslationStep, 1.0f);
        Client.SetGizmoSetting(EGizmoSetting::RotationStep, 15.0f);
        Client.SetGizmoSetting(EGizmoSetting::ScaleStep, 0.1f);
        Client.SetGizmoSetting(EGizmoSetting::Size, 1.0f);

        for (EViewportCommand Command : {EViewportCommand::ToggleTranslationSnap, EViewportCommand::ToggleRotationSnap, EViewportCommand::ToggleScaleSnap}) {
            if (Client.IsCommandChecked(Command)) {
                Client.ExecuteCommand(Command);
            }
        }

        Client.ExecuteCommand(EViewportCommand::Translate);
        Client.ExecuteCommand(EViewportCommand::WorldSpace);
        FString Output{};
        Viewport->GetStatistics()->ExecuteCommand("stat none", Output);
    }

    mSettingsDirty = true;
}
