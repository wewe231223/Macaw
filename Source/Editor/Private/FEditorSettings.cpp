/**
 * @file FEditorSettings.cpp
 * @brief 에디터 설정의 표시·직렬화·지연 저장과 테마 적용을 관리합니다.
 * @ingroup Editor
 */

#include "FEditorSettings.h"
#include "Core/Literals/AngleFunctions.h"
#include "Editor/Widget/FEditorButtonWidget.h"
#include "Editor/Widget/FEditorSliderWidget.h"
#include "Editor/Widget/FEditorCheckBoxWidget.h"
#include "Editor/Widget/FEditorTextInputWidget.h"
#include "EditorStyle.h"
#include "FEditorDrawerManager.h"
#include "FEditorWorkspace.h"
#include "FEditorViewportLayout.h"
#include "BufferVisualization.h"
#include "Editor/FImGuiUIScale.h"
#include "Editor/FEditorTabManager.h"
#include "Editor/FEditorTab.h"
#include "Core/FilePath.h"
#include "Core/Stats.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorViewport.h"
#include "Editor/FPlayInEditorSession.h"
#include "Engine/FProject.h"
#include "Platform/Window.h"
#include "RenderCore/IRendererModule.h"
#include "Serialization/JsonArchive.h"
#include "Core/Types.h"
#include <imgui_internal.h>
#include <commdlg.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <utility>

using namespace Core::Literals;

using namespace Editor;

/**
 * @brief FEditorSettings 구현에 한정되는 내부 형식·상수·도우미 함수입니다.
 */
namespace {
    /**
     * @brief 뷰포트 설정 UI와 직렬화에서 공유하는 표시 이름·ShowFlag 멤버 포인터 목록입니다.
     */
    const Core::TStaticArray<std::pair<const char*, bool Engine::FEngineShowFlags::*>, 15> ShowOptions{{{"Primitives", &Engine::FEngineShowFlags::mPrimitives},
        {"Sprites", &Engine::FEngineShowFlags::mSprites},
        {"Text", &Engine::FEngineShowFlags::mText},
        {"Shadows", &Engine::FEngineShowFlags::mShadows},
        {"Sky", &Engine::FEngineShowFlags::mSky},
        {"Fog", &Engine::FEngineShowFlags::mFog},
        {"Depth of Field", &Engine::FEngineShowFlags::mDepthOfField},
        {"FXAA", &Engine::FEngineShowFlags::mFXAA},
        {"Fog Icons", &Engine::FEngineShowFlags::mFogIcons},
        {"Light Icons", &Engine::FEngineShowFlags::mLightIcons},
        {"Light Visualizers", &Engine::FEngineShowFlags::mLightVisualizers},
        {"Bounds", &Engine::FEngineShowFlags::mBounds},
        {"Collision", &Engine::FEngineShowFlags::mCollision},
        {"Grid", &Engine::FEngineShowFlags::mGrid},
        {"Axes", &Engine::FEngineShowFlags::mAxes}}};

    /**
     * @brief 버퍼 시각화를 제외한 컬러 조명·와이어프레임 표시 모드 목록입니다.
     */
    const Core::TStaticArray<Engine::EViewMode, 6> ViewModes{Engine::EViewMode::Lit_Gouraud, Engine::EViewMode::Lit_Lambert, Engine::EViewMode::Lit_Phong, Engine::EViewMode::Unlit, Engine::EViewMode::Wireframe, Engine::EViewMode::PhysicsBodies};

    /**
     * @brief 컬러 표시 모드 목록과 같은 순서로 보여 줄 설정 UI 이름입니다.
     */
    const Core::TStaticArray<const char*, ViewModes.size()> ViewModeNames{"Lit - Gouraud", "Lit - Lambert", "Lit - Phong", "Unlit", "Wireframe", "Physics Bodies"};

    /**
     * @brief 카메라 조작·표시·변환 스냅·통계 그룹 설정을 뷰포트별로 저장하거나 복원합니다.
     * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
     * @param[in,out] Client 설정을 조회하고 변경할 뷰포트 클라이언트입니다.
     * @param[in,out] Statistics 저장하거나 복원할 뷰포트 통계 그룹 설정입니다.
     */
    void SerializeViewport(Serialization::FJsonArchive& Archive, Editor::FEditorViewportClient& Client, Editor::FViewportStatistics& Statistics) {
        Core::FSpeed Speed{Client.GetCameraSpeed()};
        float Sensitivity{Client.GetCameraSensitivity()};
        float Spacing{Client.GetGridSpacing()};
        float Thickness{Client.GetLineThickness()};
        float Width{Client.GetOrthographicWidth()};
        Int32 Mode{static_cast<Int32>(Client.GetViewMode())};
        Int32 ColorMode{static_cast<Int32>(Client.GetColorViewMode())};
        Editor::FGizmoSettings Gizmo{*Client.GetGizmoSettings()};
        Int32 TransformMode{static_cast<Int32>(Client.GetGizmoMode())};
        Int32 Space{static_cast<Int32>(Client.GetGizmoSpace())};
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
        Archive.Serialize("RotationStep", Gizmo.mRotationStep, Core::QuantityUnits::Degree);
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
        Client.SetTranslationSnapStep(Gizmo.mTranslationStep);
        Client.SetRotationSnapStep(Gizmo.mRotationStep);
        Client.SetGizmoSetting(Editor::EGizmoSetting::ScaleStep, Gizmo.mScaleStep);
        Client.SetGizmoSetting(Editor::EGizmoSetting::Size, Gizmo.mSize);

        for (const auto& [Command, Enabled] : {std::pair{Editor::EViewportCommand::ToggleTranslationSnap, Gizmo.mTranslationSnap}, std::pair{Editor::EViewportCommand::ToggleRotationSnap, Gizmo.mRotationSnap}, std::pair{Editor::EViewportCommand::ToggleScaleSnap, Gizmo.mScaleSnap}}) {
            if (Client.IsCommandChecked(Command) != Enabled) {
                Client.ExecuteCommand(Command);
            }
        }

        const Core::TStaticArray<Editor::EViewportCommand, 4> Commands{Editor::EViewportCommand::Translate, Editor::EViewportCommand::Rotate, Editor::EViewportCommand::Scale, Editor::EViewportCommand::Select};

        if (TransformMode >= 0 && static_cast<size_t>(TransformMode) < Commands.size()) {
            Client.ExecuteCommand(Commands[TransformMode]);
        }

        if (Space == static_cast<Int32>(Editor::EGizmoSpace::World) || Space == static_cast<Int32>(Editor::EGizmoSpace::Local)) {
            Client.ExecuteCommand(Space == static_cast<Int32>(Editor::EGizmoSpace::World) ? Editor::EViewportCommand::WorldSpace : Editor::EViewportCommand::LocalSpace);
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

FEditorSettings::FEditorSettings(FEditorContext* Context, Platform::FWindowsApplication* Platform, FPlayInEditorSession* PlaySession, FEditorViewportLayout& ViewportLayout, FEditorWorkspace& Workspace, FEditorTabManager& Tabs, FEditorDrawerManager& Drawers, FImGuiUIScale& UIScale, Core::FString IniPath)
	: mContext{Context},
	  mPlatform{Platform},
	  mPlaySession{PlaySession},
	  mViewportLayout{ViewportLayout},
	  mWorkspace{Workspace},
	  mTabs{Tabs},
	  mDrawers{Drawers},
	  mUIScale{UIScale},
	  mIniPath{std::move(IniPath)} {
    InitializeWidgets();
}

FEditorSettings::~FEditorSettings() = default;

void FEditorSettings::InitializeWidgets() {
    InitializeActionWidgets();

    InitializeInterfaceWidgets();
    InitializeInputWidgets();

    InitializeViewportWidgets();
    InitializeGizmoWidgets();

    InitializeRenderingWidgets();
    InitializePanelWidgets();
    InitializeProjectWidgets();
}

void FEditorSettings::InitializeActionWidgets() {
    RegisterButton("Save Settings", &FEditorSettings::SaveSettingsAction);
    RegisterButton("Reset to Defaults", &FEditorSettings::OpenResetSettingsPopup);
    RegisterButton("Close", &FEditorSettings::CloseSettings);

    RegisterButton("Reset", &FEditorSettings::ConfirmResetSettings);
    RegisterButton("Cancel", &FEditorSettings::CloseSettingsPopup);
}

void FEditorSettings::InitializeInterfaceWidgets() {
    RegisterTextInput("Search", "Filter themes...", &FEditorSettings::GetThemeFilter, &FEditorSettings::SetThemeFilter, mThemeFilter.size());
    RegisterButton("Import ImThemes TOML...", &FEditorSettings::ImportTheme);

    RegisterCheckBox("Automatic UI Scale", &FEditorSettings::IsAutomaticUIScale, &FEditorSettings::SetAutomaticUIScale);
    RegisterSlider("UI Scale", 0.75f, 3.0f, &FEditorSettings::GetUserUIScale, &FEditorSettings::SetUserUIScale, "%.2fx");
    mWidgets.at("UI Scale")->SetEnabledAttribute(std::bind_front(&FEditorSettings::IsManualUIScale, this));

    RegisterSlider("Font Size", 10.0f, 32.0f, &FEditorSettings::GetFontSize, &FEditorSettings::SetFontSize, "%.0f px");
}

void FEditorSettings::InitializeInputWidgets() {
    RegisterCheckBox("Keyboard Navigation", &FEditorSettings::IsKeyboardNavigationEnabled, &FEditorSettings::SetKeyboardNavigationEnabled);
    RegisterCheckBox("Move Windows from Title Bar Only", &FEditorSettings::IsTitleBarMoveOnly, &FEditorSettings::SetTitleBarMoveOnly);

    RegisterInputSlider("Double Click Time", &ImGuiIO::MouseDoubleClickTime, (0.1_s).GetBase(), (1.0_s).GetBase(), "%.2f s");
    RegisterInputSlider("Mouse Drag Threshold", &ImGuiIO::MouseDragThreshold, 1.0f, 20.0f, "%.0f px");

    RegisterInputSlider("Key Repeat Delay", &ImGuiIO::KeyRepeatDelay, (0.1_s).GetBase(), (1.0_s).GetBase(), "%.2f s");
    RegisterInputSlider("Key Repeat Rate", &ImGuiIO::KeyRepeatRate, (0.02_s).GetBase(), (0.3_s).GetBase(), "%.2f s");
}

void FEditorSettings::InitializeViewportWidgets() {
    RegisterSlider("Horizontal Split", 0.1f, 0.9f, &FEditorSettings::GetHorizontalSplit, &FEditorSettings::SetHorizontalSplit, "%.2f");
    RegisterSlider("Vertical Split", 0.1f, 0.9f, &FEditorSettings::GetVerticalSplit, &FEditorSettings::SetVerticalSplit, "%.2f");

    RegisterSlider("Movement Speed", (0.1_mps).GetBase(), (128_mps).GetBase(), [this]() {
        return GetSettingsClient().GetCameraSpeed().GetBase();
    }, [this](float Value) {
        GetSettingsClient().SetCameraSpeed(Core::FSpeed::FromBase(Value));
    }, "%.2f m/s", 0.01f);

    RegisterViewportSlider("Mouse Sensitivity", Core::Math::ToDegrees(0.01_deg), Core::Math::ToDegrees(2_deg), &FEditorViewportClient::GetCameraSensitivity, &FEditorViewportClient::SetCameraSensitivity, "%.3f", 0.001f);
    RegisterViewportSlider("Orthographic Width", (0.01_m).GetBase(), (100000_m).GetBase(), &FEditorViewportClient::GetOrthographicWidth, &FEditorViewportClient::SetOrthographicWidth, "%.2f", 0.001f);

    RegisterSlider("Field of View", Core::Math::ToDegrees(1_deg), Core::Math::ToDegrees(179_deg), &FEditorSettings::GetFieldOfView, &FEditorSettings::SetFieldOfView, "%.1f degrees");
    RegisterSlider("Near Clip", (0.001_m).GetBase(), (999999_m).GetBase(), &FEditorSettings::GetNearClip, &FEditorSettings::SetNearClip, "%.3f", 0.01f);
    RegisterSlider("Far Clip", (0.002_m).GetBase(), (1000000_m).GetBase(), &FEditorSettings::GetFarClip, &FEditorSettings::SetFarClip, "%.1f", 10.0f);

    RegisterViewportSlider("Grid Spacing", (0.01_m).GetBase(), (1000_m).GetBase(), &FEditorViewportClient::GetGridSpacing, &FEditorViewportClient::SetGridSpacing, "%.2f", 0.001f);
    RegisterViewportSlider("Line Thickness", 0.5f, 10.0f, &FEditorViewportClient::GetLineThickness, &FEditorViewportClient::SetLineThickness, "%.1f px", 0.05f);

    for (const auto& [Name, Member] : ShowOptions) {
        RegisterCheckBox(Name, Name, std::bind_front(&FEditorSettings::GetShowOption, this, Member), std::bind_front(&FEditorSettings::SetShowOption, this, Member));
    }
}

void FEditorSettings::InitializeGizmoWidgets() {
    for (const auto& [Label, Command] : {std::pair{"Translation Snap", EViewportCommand::ToggleTranslationSnap}, std::pair{"Rotation Snap", EViewportCommand::ToggleRotationSnap}, std::pair{"Scale Snap", EViewportCommand::ToggleScaleSnap}}) {
        RegisterCheckBox(Label, Label, std::bind_front(&FEditorSettings::IsViewportCommandChecked, this, Command), std::bind_front(&FEditorSettings::SetViewportCommandChecked, this, Command));
    }

    RegisterGizmoSlider("Translation Step", EGizmoSetting::TranslationStep, (0.001_m).GetBase(), (10000_m).GetBase(), "%.3f", (0.01_m).GetBase());
    RegisterGizmoSlider("Rotation Step", EGizmoSetting::RotationStep, Core::Math::ToDegrees(0.001_deg), Core::Math::ToDegrees(360_deg), "%.3f degrees", Core::Math::ToDegrees(0.1_deg));

    RegisterGizmoSlider("Scale Step", EGizmoSetting::ScaleStep, 0.001f, 100.0f, "%.3f", 0.01f);
    RegisterGizmoSlider("Gizmo Size", EGizmoSetting::Size, 0.5f, 2.0f, "%.2fx");
}

void FEditorSettings::InitializeRenderingWidgets() {
    RegisterCheckBox("VSync", &FEditorSettings::IsVSyncEnabled, &FEditorSettings::SetVSyncEnabled);

    RegisterButton("Reload Changed Shaders", &FEditorSettings::ReloadChangedShaders);
    RegisterButton("Open Shadow Atlas", &FEditorSettings::OpenShadowAtlas);
}

void FEditorSettings::InitializePanelWidgets() {
    RegisterCheckBox("Statistics Window", &FEditorSettings::IsStatisticsWindowVisible, &FEditorSettings::SetStatisticsWindowVisible);

    for (const char* Id : {"PlaceActors", "Outliner", "Details"}) {
        const Core::FString TabId{Id};
        const Core::FString Label{mTabs.FindTab(TabId)->GetName()};

        RegisterCheckBox(TabId, Label, std::bind_front(&FEditorSettings::IsTabVisible, this, TabId), std::bind_front(&FEditorSettings::SetTabVisible, this, TabId));
    }
}

void FEditorSettings::InitializeProjectWidgets() {
    RegisterTextInput("Content Path", "", &FEditorSettings::GetStartupScene, &FEditorSettings::SetStartupScene, mStartupScene.size());

    RegisterButton("Save Startup Scene", &FEditorSettings::SaveStartupScene);
}

void FEditorSettings::RegisterWidget(std::unique_ptr<FEditorWidget> Widget) {
    const Core::FString Id{Widget->GetId()};

    mWidgets.emplace(Id, std::move(Widget));
}

void FEditorSettings::RegisterButton(Core::FString Label, FAction Handler) {
    RegisterWidget(std::make_unique<FEditorButtonWidget>(Label, Label, std::bind_front(Handler, this)));
}

void FEditorSettings::RegisterSlider(Core::FString Label, float Minimum, float Maximum, FFloatGetter Getter, FFloatSetter Setter, Core::FString Format, float DragSpeed) {
    RegisterSlider(std::move(Label), Minimum, Maximum, std::bind_front(Getter, this), std::bind_front(Setter, this), std::move(Format), DragSpeed);
}

void FEditorSettings::RegisterSlider(Core::FString Label, float Minimum, float Maximum, std::function<float()> Getter, std::function<void(float)> Setter, Core::FString Format, float DragSpeed) {
    auto Widget{std::make_unique<FEditorSliderWidget>(Label, Label, Minimum, Maximum, std::move(Getter), std::bind_front(&FEditorSettings::ApplyFloatChange, this, std::move(Setter)), std::move(Format))};

    Widget->SetDragSpeed(DragSpeed);
    RegisterWidget(std::move(Widget));
}

void FEditorSettings::RegisterCheckBox(Core::FString Label, FBoolGetter Getter, FBoolSetter Setter) {
    RegisterCheckBox(Label, Label, std::bind_front(Getter, this), std::bind_front(Setter, this));
}

void FEditorSettings::RegisterCheckBox(Core::FString Id, Core::FString Label, std::function<bool()> Getter, std::function<void(bool)> Setter) {
    RegisterWidget(std::make_unique<FEditorCheckBoxWidget>(std::move(Id), std::move(Label), std::move(Getter), std::bind_front(&FEditorSettings::ApplyBoolChange, this, std::move(Setter))));
}

void FEditorSettings::RegisterTextInput(Core::FString Label, Core::FString Hint, FTextGetter Getter, FTextSetter Setter, Index_t Capacity) {
    RegisterWidget(std::make_unique<FEditorTextInputWidget>(Label, Label, std::move(Hint), std::bind_front(Getter, this), std::bind_front(Setter, this), Capacity));
}

void FEditorSettings::RegisterInputSlider(Core::FString Label, FInputFloat Member, float Minimum, float Maximum, Core::FString Format) {
    RegisterSlider(std::move(Label), Minimum, Maximum, std::bind_front(&FEditorSettings::GetInputFloat, this, Member), std::bind_front(&FEditorSettings::SetInputFloat, this, Member), std::move(Format));
}

void FEditorSettings::RegisterViewportSlider(Core::FString Label, float Minimum, float Maximum, FViewportFloatGetter Getter, FViewportFloatSetter Setter, Core::FString Format, float DragSpeed) {
    RegisterSlider(std::move(Label), Minimum, Maximum, std::bind_front(&FEditorSettings::GetViewportFloat, this, Getter), std::bind_front(&FEditorSettings::SetViewportFloat, this, Setter), std::move(Format), DragSpeed);
}

void FEditorSettings::RegisterGizmoSlider(Core::FString Label, Editor::EGizmoSetting Setting, float Minimum, float Maximum, Core::FString Format, float DragSpeed) {
    RegisterSlider(std::move(Label), Minimum, Maximum, std::bind_front(&FEditorSettings::GetGizmoFloat, this, Setting), std::bind_front(&FEditorSettings::SetGizmoFloat, this, Setting), std::move(Format), DragSpeed);
}

void FEditorSettings::RegisterStatisticsWidget(const Core::FString& Group) {
    const Core::FString Id{"Stat:" + Group};

    if (mWidgets.contains(Id)) {
        return;
    }

    RegisterWidget(std::make_unique<FEditorCheckBoxWidget>(Id, Group, std::bind_front(&FEditorSettings::IsStatisticEnabled, this, Group), std::bind_front(&FEditorSettings::SetStatisticEnabled, this, Group)));
}

void FEditorSettings::ApplyFloatChange(const std::function<void(float)>& Setter, float Value) {
    Setter(Value);
    MarkDirty();
}

void FEditorSettings::ApplyBoolChange(const std::function<void(bool)>& Setter, bool Value) {
    Setter(Value);
    MarkDirty();
}

void FEditorSettings::SaveSettingsAction() {
    SaveSettings();
}

void FEditorSettings::OpenResetSettingsPopup() {
    ImGui::OpenPopup("Reset Settings?");
}

void FEditorSettings::CloseSettings() {
    mSettingsOpen = false;
}

void FEditorSettings::ConfirmResetSettings() {
    ResetSettings();
    CloseSettingsPopup();
}

void FEditorSettings::CloseSettingsPopup() {
    ImGui::CloseCurrentPopup();
}

void FEditorSettings::ReloadChangedShaders() {
    mSettingsStatus = mContext->GetRenderer()->ReloadChangedShaders() ? "Shaders reloaded" : "Shader reload failed; see Console";
}

void FEditorSettings::OpenShadowAtlas() {
    mContext->SetShadowAtlasOpen(true);
}

void FEditorSettings::SaveStartupScene() {
    Core::FString Error{};

    mSettingsStatus = mContext->GetProject()->SetStartupScene(Core::FilePath::FromUtf8(mStartupScene.data()), Error) ? "Startup scene saved" : Error;
}

Core::FString FEditorSettings::GetThemeFilter() const {
    return {mThemeFilter.data()};
}

void FEditorSettings::SetThemeFilter(const Core::FString& Value) {
    const Index_t Count{std::min(Value.size(), mThemeFilter.size() - 1)};

    std::copy_n(Value.data(), Count, mThemeFilter.data());
    mThemeFilter[Count] = '\0';
}

Core::FString FEditorSettings::GetStartupScene() const {
    return {mStartupScene.data()};
}

void FEditorSettings::SetStartupScene(const Core::FString& Value) {
    const Index_t Count{std::min(Value.size(), mStartupScene.size() - 1)};

    std::copy_n(Value.data(), Count, mStartupScene.data());
    mStartupScene[Count] = '\0';
}

bool FEditorSettings::IsAutomaticUIScale() const {
    return mUIScale.GetUserScale() == 0.0f;
}

void FEditorSettings::SetAutomaticUIScale(bool Automatic) {
    mUIScale.SetUserScale(Automatic ? 0.0f : 1.0f);
}

bool FEditorSettings::IsManualUIScale() const {
    return !IsAutomaticUIScale();
}

float FEditorSettings::GetUserUIScale() const {
    return IsAutomaticUIScale() ? 1.0f : mUIScale.GetUserScale();
}

void FEditorSettings::SetUserUIScale(float Scale) {
    mUIScale.SetUserScale(Scale);
}

float FEditorSettings::GetFontSize() const {
    return mFontSize;
}

void FEditorSettings::SetFontSize(float Size) {
    mFontSize = Size;
    mAppearanceDirty = true;
}

bool FEditorSettings::IsKeyboardNavigationEnabled() const {
    return (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;
}

void FEditorSettings::SetKeyboardNavigationEnabled(bool Enabled) {
    ImGuiIO& Input{ImGui::GetIO()};

    Input.ConfigFlags = Enabled ? Input.ConfigFlags | ImGuiConfigFlags_NavEnableKeyboard : Input.ConfigFlags & ~ImGuiConfigFlags_NavEnableKeyboard;
}

bool FEditorSettings::IsTitleBarMoveOnly() const {
    return ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly;
}

void FEditorSettings::SetTitleBarMoveOnly(bool Enabled) {
    ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = Enabled;
}

float FEditorSettings::GetInputFloat(FInputFloat Member) const {
    return ImGui::GetIO().*Member;
}

void FEditorSettings::SetInputFloat(FInputFloat Member, float Value) {
    ImGui::GetIO().*Member = Value;
}

float FEditorSettings::GetHorizontalSplit() const {
    return mViewportLayout.GetHorizontalRatio();
}

void FEditorSettings::SetHorizontalSplit(float Value) {
    mViewportLayout.SetSplitRatios(Value, mViewportLayout.GetVerticalRatio());
}

float FEditorSettings::GetVerticalSplit() const {
    return mViewportLayout.GetVerticalRatio();
}

void FEditorSettings::SetVerticalSplit(float Value) {
    mViewportLayout.SetSplitRatios(mViewportLayout.GetHorizontalRatio(), Value);
}

float FEditorSettings::GetViewportFloat(FViewportFloatGetter Getter) const {
    return (GetSettingsClient().*Getter)();
}

void FEditorSettings::SetViewportFloat(FViewportFloatSetter Setter, float Value) {
    (GetSettingsClient().*Setter)(Value);
}

float FEditorSettings::GetFieldOfView() const {
    return Core::Math::ToDegrees(GetSettingsClient().GetPerspectiveCamera().GetFieldOfView());
}

void FEditorSettings::SetFieldOfView(float Value) {
    Engine::FEditorCameraState Camera{GetSettingsClient().GetPerspectiveCamera()};

    if (Camera.Set(Camera.GetLocation(), Camera.GetRotation(), Value * 1_deg, Camera.GetNearClip(), Camera.GetFarClip())) {
        GetSettingsClient().SetPerspectiveCamera(Camera);
    }
}

float FEditorSettings::GetNearClip() const {
    return GetSettingsClient().GetPerspectiveCamera().GetNearClip().GetBase();
}

void FEditorSettings::SetNearClip(float Value) {
    Engine::FEditorCameraState Camera{GetSettingsClient().GetPerspectiveCamera()};

    if (Camera.Set(Camera.GetLocation(), Camera.GetRotation(), Camera.GetFieldOfView(), std::min(Core::FLength::FromBase(Value), Camera.GetFarClip() - 0.001_m), Camera.GetFarClip())) {
        GetSettingsClient().SetPerspectiveCamera(Camera);
    }
}

float FEditorSettings::GetFarClip() const {
    return GetSettingsClient().GetPerspectiveCamera().GetFarClip().GetBase();
}

void FEditorSettings::SetFarClip(float Value) {
    Engine::FEditorCameraState Camera{GetSettingsClient().GetPerspectiveCamera()};

    if (Camera.Set(Camera.GetLocation(), Camera.GetRotation(), Camera.GetFieldOfView(), Camera.GetNearClip(), std::max(Core::FLength::FromBase(Value), Camera.GetNearClip() + 0.001_m))) {
        GetSettingsClient().SetPerspectiveCamera(Camera);
    }
}

float FEditorSettings::GetGizmoFloat(Editor::EGizmoSetting Setting) const {
    return GetSettingsClient().GetGizmoSettingValue(Setting);
}

void FEditorSettings::SetGizmoFloat(EGizmoSetting Setting, float Value) {
    GetSettingsClient().SetGizmoSetting(Setting, Value);
}

bool FEditorSettings::GetShowOption(FShowFlag Member) const {
    return GetSettingsClient().GetShowFlags().*Member;
}

void FEditorSettings::SetShowOption(FShowFlag Member, bool Value) {
    Engine::FEngineShowFlags Flags{GetSettingsClient().GetShowFlags()};
    Flags.*Member = Value;

    GetSettingsClient().SetShowFlags(Flags);
}

bool FEditorSettings::IsViewportCommandChecked(EViewportCommand Command) const {
    return GetSettingsClient().IsCommandChecked(Command);
}

void FEditorSettings::SetViewportCommandChecked(EViewportCommand Command, bool Enabled) {
    if (IsViewportCommandChecked(Command) != Enabled) {
        GetSettingsClient().ExecuteCommand(Command);
    }
}

bool FEditorSettings::IsVSyncEnabled() const {
    return !mContext->GetRenderer()->IsTearingAllowed();
}

void FEditorSettings::SetVSyncEnabled(bool Enabled) {
    mContext->GetRenderer()->SetAllowTearing(!Enabled);
}

bool FEditorSettings::IsStatisticsWindowVisible() const {
    return mShowStatistics;
}

void FEditorSettings::SetStatisticsWindowVisible(bool Visible) {
    mShowStatistics = Visible;
}

bool FEditorSettings::IsTabVisible(const Core::FString& Id) const {
    return mTabs.IsVisible(Id);
}

void FEditorSettings::SetTabVisible(const Core::FString& Id, bool Visible) {
    mTabs.SetVisible(Id, Visible);
}

bool FEditorSettings::IsStatisticEnabled(const Core::FString& Group) const {
    return mViewportLayout.GetViewport(mSettingsViewport)->GetStatistics()->IsEnabled(Group);
}

void FEditorSettings::SetStatisticEnabled(const Core::FString& Group, bool Enabled) {
    Core::FString Output{};

    mSettingsDirty |= mViewportLayout.GetViewport(mSettingsViewport)->GetStatistics()->ExecuteCommand("stat " + Group + (Enabled ? " on" : " off"), Output);
}

void FEditorSettings::DrawWidget(const char* Id) {
    mWidgets.at(Id)->Draw(ImGui::GetIO().DeltaTime);
}

FEditorViewportClient& FEditorSettings::GetSettingsClient() const {
    return *mViewportLayout.GetViewport(mSettingsViewport)->GetClient();
}

void FEditorSettings::MarkDirty() {
    mSettingsDirty = true;
}

bool& FEditorSettings::GetStatisticsVisibility() {
    return mShowStatistics;
}

void FEditorSettings::ApplyAppearance() {
    if (mAppearanceDirty) {
        mUIScale.SetBaseStyle(mThemes.GetStyle(mThemeIndex), mFontSize);
        mAppearanceDirty = false;
    }

    mUIScale.Update(*mPlatform);
}

void FEditorSettings::Tick(float DeltaTime) {
    if (mSettingsDirty) {
        mSettingsSaveDelay += DeltaTime;
        if (mSettingsSaveDelay >= (2_s).GetBase()) {
            SaveSettings();
            mSettingsSaveDelay = 0.0f;
        }
    }
}

void FEditorSettings::InitializeSettings() {
    mThemes = Editor::FImGuiThemeCatalog{};
    mThemeIndex = 0;
    mFontSize = 16.0f;
    mTabs.SetVisible("Content", false);
    mTabs.SetVisible("Console", false);
    mWorkspace.SetLayoutVersion(0);
    mUIScale.SetUserScale(0.0f);
    mSettingsPath = mContext->GetProject()->GetRootPath() / "Saved/EditorSettings.json";
    mSettingsStatus.clear();
    mSettingsDirty = false;
    mSettingsSaveDelay = 0.0f;
    mSettingsViewport = 0;
    mPlaySession->SetOptions({});
    mThemeFilter.fill('\0');
    strncpy_s(mStartupScene.data(), mStartupScene.size(), mContext->GetProject()->GetStartupScene().c_str(), _TRUNCATE);

    Core::TStaticArray<wchar_t, 32768> Executable{};
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

void FEditorSettings::LoadSettings() {
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

bool FEditorSettings::SaveSettings() {
    if (mSettingsPath.empty()) {
        return false;
    }

    Serialization::FJsonArchive Archive{Core::EArchiveMode::Saving};
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

void FEditorSettings::SerializeSettings(Serialization::FJsonArchive& Archive) {
    Core::FString Theme{mThemes.GetName(mThemeIndex)};
    float FontSize{mFontSize};
    float Scale{mUIScale.GetUserScale()};
    bool Tearing{mContext->GetRenderer()->IsTearingAllowed()};
    bool PlaceActors{mTabs.IsVisible("PlaceActors")};
    bool Outliner{mTabs.IsVisible("Outliner")};
    bool Details{mTabs.IsVisible("Details")};
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
    Archive.Serialize("Statistics", Statistics);

    Uint32 LayoutVersion{mWorkspace.GetLayoutVersion()};

    if (!Archive.IsLoading() || Archive.HasField("LayoutVersion")) {
        Archive.Serialize("LayoutVersion", LayoutVersion);
    }

    Archive.Serialize("KeyboardNavigation", KeyboardNavigation);
    Archive.Serialize("TitleBarOnly", TitleBarOnly);
    Archive.Serialize("DoubleClickTime", DoubleClickTime);
    Archive.Serialize("DragThreshold", DragThreshold);
    Archive.Serialize("KeyRepeatDelay", RepeatDelay);
    Archive.Serialize("KeyRepeatRate", RepeatRate);

    if ((!Archive.IsLoading() || Archive.HasField("PlayInEditor")) && Archive.BeginObject("PlayInEditor")) {
        FPlayInEditorSession::FOptions Options{mPlaySession->GetOptions()};

        Archive.Serialize("NewWindow", Options.mNewWindow);
        Archive.Serialize("StartAtEditorCamera", Options.mStartAtEditorCamera);
        Archive.Serialize("WindowWidth", Options.mWindowWidth);
        Archive.Serialize("WindowHeight", Options.mWindowHeight);
        Archive.Serialize("CaptureInput", Options.mCaptureInput);
        Archive.Serialize("StartPaused", Options.mStartPaused);
        Archive.EndObject();

        if (Archive.IsLoading() && !Archive.HasError()) {
            mPlaySession->SetOptions(Options);
        }
    }

    if (Archive.IsLoading() && !Archive.HasError()) {
        mThemeIndex = mThemes.Find(Theme);
        mFontSize = std::isfinite(FontSize) && FontSize >= 10.0f && FontSize <= 32.0f ? FontSize : 16.0f;
        mUIScale.SetUserScale(Scale);
        mAppearanceDirty = true;
        mContext->GetRenderer()->SetAllowTearing(Tearing);
        mTabs.SetVisible("PlaceActors", PlaceActors);
        mTabs.SetVisible("Outliner", Outliner);
        mTabs.SetVisible("Details", Details);
        mShowStatistics = Statistics;
        mWorkspace.SetLayoutVersion(LayoutVersion);
        Input.ConfigFlags = KeyboardNavigation ? Input.ConfigFlags | ImGuiConfigFlags_NavEnableKeyboard : Input.ConfigFlags & ~ImGuiConfigFlags_NavEnableKeyboard;
        Input.ConfigWindowsMoveFromTitleBarOnly = TitleBarOnly;
        Input.MouseDoubleClickTime = std::isfinite(DoubleClickTime) ? std::clamp(DoubleClickTime, (0.1_s).GetBase(), (1_s).GetBase()) : (0.3_s).GetBase();
        Input.MouseDragThreshold = std::isfinite(DragThreshold) ? std::clamp(DragThreshold, 1.0f, 20.0f) : 6.0f;
        Input.KeyRepeatDelay = std::isfinite(RepeatDelay) ? std::clamp(RepeatDelay, (0.1_s).GetBase(), (1_s).GetBase()) : (0.275_s).GetBase();
        Input.KeyRepeatRate = std::isfinite(RepeatRate) ? std::clamp(RepeatRate, (0.02_s).GetBase(), (0.3_s).GetBase()) : (0.05_s).GetBase();
    }

    mDrawers.SerializeSettings(Archive);

    for (Index_t Index{}; Index < 4; ++Index) {
        Editor::FEditorViewport* Viewport{mViewportLayout.GetViewport(Index)};

        if (Viewport != nullptr && Archive.BeginObject("Viewport" + std::to_string(Index))) {
            SerializeViewport(Archive, *Viewport->GetClient(), *Viewport->GetStatistics());
            Archive.EndObject();
        }
    }
}

void FEditorSettings::DrawSettingsMenu() {
    if (ImGui::MenuItem("Editor Preferences...", "Ctrl+,")) {
        OpenSettings(0);
    }

    if (ImGui::MenuItem("Project Settings...")) {
        OpenSettings(5);
    }

    ImGui::Separator();

    if (ImGui::BeginMenu("Theme")) {
        for (Index_t Index{}; Index < mThemes.GetCount(); ++Index) {
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
}

void FEditorSettings::DrawSettings() {
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
            for (Index_t Index{}; Index < static_cast<int>(std::size(Sections)); ++Index) {
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
                const Core::TStaticArray<const char*, 4> Names{"Viewport 1", "Viewport 2", "Viewport 3", "Viewport 4"};
                Int32 Selected{static_cast<Int32>(mSettingsViewport)};

                if (ImGui::Combo("Viewport", &Selected, Names.data(), static_cast<int>(Names.size()))) {
                    mSettingsViewport = static_cast<Index_t>(Selected);
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

        DrawWidget("Save Settings");

        ImGui::SameLine();

        DrawWidget("Reset to Defaults");

        ImGui::SameLine();

        DrawWidget("Close");

        if (ImGui::BeginPopupModal("Reset Settings?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("Reset editor preferences for this project?");
            ImGui::BeginDisabled(!mViewportLayout.CanChangeLayout());

            DrawWidget("Reset");

            ImGui::EndDisabled();
            ImGui::SameLine();

            DrawWidget("Cancel");

            ImGui::EndPopup();
        }

        ImGui::TextDisabled("%s", !mSettingsStatus.empty() ? mSettingsStatus.c_str() : mSettingsDirty ? "Changes apply immediately and save automatically." : "Preferences are saved per project.");
    }

    ImGui::End();

    if (!mSettingsOpen && mSettingsDirty) {
        SaveSettings();
    }
}

void FEditorSettings::DrawInterfaceSettings() {
    ImGui::TextDisabled("ImThemes: %u themes", mThemes.GetCount() - 1);
    DrawWidget("Search");

    if (ImGui::BeginCombo("Theme", mThemes.GetName(mThemeIndex).c_str())) {
        ImGuiTextFilter Filter{mThemeFilter.data()};

        for (Index_t Index{}; Index < mThemes.GetCount(); ++Index) {
            if (!Filter.PassFilter(mThemes.GetName(Index).c_str()) && !Filter.PassFilter(mThemes.GetAuthor(Index).c_str())) {
                continue;
            }

            ImGui::PushID(static_cast<int>(Index));

            if (ImGui::Selectable(mThemes.GetName(Index).c_str(), mThemeIndex == Index)) {
                mThemeIndex = Index;
                mAppearanceDirty = true;
                MarkDirty();
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
    DrawWidget("Import ImThemes TOML...");
    ImGui::SeparatorText("Size");
    DrawWidget("Automatic UI Scale");
    DrawWidget("UI Scale");
    ImGui::TextDisabled("Effective scale: %.2fx (includes display DPI)", mUIScale.GetScale());
    DrawWidget("Font Size");
    ImGui::SeparatorText("Input & Windows");
    DrawWidget("Keyboard Navigation");
    DrawWidget("Move Windows from Title Bar Only");
    DrawWidget("Double Click Time");
    DrawWidget("Mouse Drag Threshold");
    DrawWidget("Key Repeat Delay");
    DrawWidget("Key Repeat Rate");
}

void FEditorSettings::DrawViewportSettings() {
    FEditorViewportClient& Client{GetSettingsClient()};
    ImGui::BeginDisabled(!mViewportLayout.CanChangeLayout());

    if (ImGui::BeginCombo("Viewport Layout", FEditorViewportLayout::GetPresetName(mViewportLayout.GetPreset()))) {
        for (Index_t Index{}; Index < static_cast<Index_t>(EEditorViewportLayoutPreset::Count); ++Index) {
            const auto Preset{static_cast<EEditorViewportLayoutPreset>(Index)};

            if (ImGui::Selectable(FEditorViewportLayout::GetPresetName(Preset), mViewportLayout.GetPreset() == Preset)) {
                mViewportLayout.SetPreset(Preset);
                MarkDirty();
            }
        }

        ImGui::EndCombo();
    }

    DrawWidget("Horizontal Split");
    DrawWidget("Vertical Split");
    ImGui::SeparatorText("Camera");
    DrawWidget("Movement Speed");
    DrawWidget("Mouse Sensitivity");
    DrawWidget("Orthographic Width");
    DrawWidget("Field of View");
    DrawWidget("Near Clip");
    DrawWidget("Far Clip");
    ImGui::TextWrapped("Camera projection is stored with the scene when you save it.");
    ImGui::SeparatorText("Display");

    const Engine::EViewMode ActiveMode{Client.GetViewMode() == Engine::EViewMode::PhysicsBodies ? Engine::EViewMode::PhysicsBodies : Client.GetColorViewMode()};
    const auto Current{std::find(ViewModes.begin(), ViewModes.end(), ActiveMode)};
    int Mode{Current == ViewModes.end() ? 2 : static_cast<int>(Current - ViewModes.begin())};

    if (ImGui::Combo("View Mode", &Mode, ViewModeNames.data(), static_cast<int>(ViewModeNames.size()))) {
        Client.SetViewMode(ViewModes[Mode]);
        MarkDirty();
    }

    const auto& BufferNames{Editor::BufferVisualization::Names};
    const auto& BufferModes{Editor::BufferVisualization::Modes};
    const auto SelectedBuffer{std::find(BufferModes.begin(), BufferModes.end(), Client.GetViewMode())};
    const Index_t Buffer{SelectedBuffer == BufferModes.end() ? 0u : static_cast<Index_t>(SelectedBuffer - BufferModes.begin())};

    if (ImGui::BeginCombo("Buffer Visualization", BufferNames[Buffer])) {
        for (Index_t Index{}; Index < BufferModes.size(); ++Index) {
            ImGui::BeginDisabled(!Client.SupportsViewMode(BufferModes[Index]));

            if (ImGui::Selectable(BufferNames[Index], Index == Buffer)) {
                Client.SetViewMode(Index == 0 ? Client.GetColorViewMode() : BufferModes[Index]);
                MarkDirty();
            }

            ImGui::EndDisabled();

            if (Index == Buffer) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    DrawWidget("Grid Spacing");
    DrawWidget("Line Thickness");

    for (const auto& [Name, Member] : ShowOptions) {
        DrawWidget(Name);
    }

    ImGui::EndDisabled();
}

void FEditorSettings::DrawGizmoSettings() {
    FEditorViewportClient& Client{GetSettingsClient()};
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
    DrawWidget("Translation Snap");
    DrawWidget("Rotation Snap");
    DrawWidget("Scale Snap");
    DrawWidget("Translation Step");
    DrawWidget("Rotation Step");
    DrawWidget("Scale Step");
    DrawWidget("Gizmo Size");
    ImGui::EndDisabled();
}

void FEditorSettings::DrawRenderingSettings() {
    RenderCore::IRendererModule& Renderer{*mContext->GetRenderer()};
    ImGui::BeginDisabled(!Renderer.IsTearingSupported());
    DrawWidget("VSync");
    ImGui::EndDisabled();
    ImGui::TextWrapped("%s", Renderer.IsTearingSupported() ? "Disabling VSync allows tearing in windowed mode." : "Tearing is unavailable on this device. VSync is enabled.");
    DrawWidget("Reload Changed Shaders");
    DrawWidget("Open Shadow Atlas");
    ImGui::TextWrapped("View mode, shadows and visibility filters are available under Viewport & Camera for each viewport.");
}

void FEditorSettings::DrawPanelSettings() {
    for (const char* Id : {"PlaceActors", "Outliner", "Details"}) {
        DrawWidget(Id);
    }

    DrawWidget("Statistics Window");
    ImGui::SeparatorText("Viewport Statistics Overlay");

    for (const Core::FString& Group : Core::FStats::Get().GetGroups()) {
        const Core::FString Id{"Stat:" + Group};

        RegisterStatisticsWidget(Group);

        DrawWidget(Id.c_str());
    }
}

void FEditorSettings::DrawProjectSettings() {
    Engine::FProject& Project{*mContext->GetProject()};
    const bool ActiveForwardShading{mContext->GetRenderer()->UsesForwardShading()};
    const char* ShadingPaths[]{"Deferred", "Forward"};
    int ShadingPath{Project.UsesForwardShading() ? 1 : 0};

    ImGui::TextWrapped("Project: %s", Project.GetName().c_str());
    ImGui::TextWrapped("Location: %s", Core::FilePath::ToUtf8(Project.GetFilePath()).c_str());
    ImGui::SeparatorText("Startup Scene");
    ImGui::BeginDisabled(!mContext->CanEditWorld());
    DrawWidget("Content Path");
    DrawWidget("Save Startup Scene");
    ImGui::EndDisabled();
    ImGui::SeparatorText("Rendering");

    if (ImGui::Combo("Shading Path", &ShadingPath, ShadingPaths, static_cast<int>(std::size(ShadingPaths)))) {
        Core::FString Error{};

        mSettingsStatus = Project.SetForwardShading(ShadingPath == 1, Error) ? "Shading path saved" : Error;
    }

    if (Project.UsesForwardShading() != ActiveForwardShading) {
        ImGui::TextWrapped("Reopen the project to apply this change. Active path: %s", ShadingPaths[ActiveForwardShading ? 1 : 0]);
    }

    if (Project.UsesForwardShading() || ActiveForwardShading) {
        ImGui::TextWrapped("Decals and GBuffer visualizations require Deferred shading.");
    }

    ImGui::SeparatorText("Preferences");
    ImGui::TextWrapped("Settings: %s", Core::FilePath::ToUtf8(mSettingsPath).c_str());
    ImGui::TextWrapped("Custom themes: %s", Core::FilePath::ToUtf8(mSettingsPath.parent_path() / "Themes").c_str());
}

void FEditorSettings::ImportTheme() {
    Core::TStaticArray<wchar_t, 32768> Filename{};
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

void FEditorSettings::ResetSettings() {
    mPlaySession->SetOptions({});
    mThemeIndex = 0;
    mFontSize = 16.0f;
    mUIScale.SetUserScale(0.0f);
    mAppearanceDirty = true;
    mContext->GetRenderer()->SetAllowTearing(false);
    mTabs.SetVisible("PlaceActors", true);
    mTabs.SetVisible("Outliner", true);
    mTabs.SetVisible("Details", true);
    mTabs.SetVisible("Content", false);
    mTabs.SetVisible("Console", false);
    mShowStatistics = false;

    ImGuiIO& Input{ImGui::GetIO()};
    Input.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    Input.ConfigWindowsMoveFromTitleBarOnly = false;
    Input.MouseDoubleClickTime = (0.3_s).GetBase();
    Input.MouseDragThreshold = 6.0f;
    Input.KeyRepeatDelay = (0.275_s).GetBase();
    Input.KeyRepeatRate = (0.05_s).GetBase();

    mViewportLayout.SetPreset(EEditorViewportLayoutPreset::Single);
    mViewportLayout.SetSplitRatios(0.5f, 0.5f);

    for (Index_t Index{}; Index < 4; ++Index) {
        Editor::FEditorViewport* Viewport{mViewportLayout.GetViewport(Index)};
        Editor::FEditorViewportClient& Client{*Viewport->GetClient()};
        Client.SetCameraSpeed(4_mps);
        Client.SetCameraSensitivity(Core::Math::ToDegrees(0.2_deg));
        Client.SetGridSpacing((1_m).GetBase());
        Client.SetLineThickness(1.5f);
        Client.SetOrthographicWidth((20_m).GetBase());
        Client.SetViewMode(Engine::EViewMode::Lit_Phong);
        Client.SetShowFlags(Engine::FEngineShowFlags{});
        Client.SetTranslationSnapStep(1_m);
        Client.SetRotationSnapStep(15_deg);
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

void FEditorSettings::OpenSettings(Index_t Section) {
    mSettingsSection = Section;
    mSettingsOpen = true;
    mFocusSettings = true;
}
