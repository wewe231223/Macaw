/**
 * @file FEditorSettings.h
 * @brief 에디터 설정의 표시·직렬화·지연 저장과 테마 적용을 관리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "FImGuiThemeCatalog.h"
#include "Editor/EViewportCommand.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorTabManager.h"
#include "Editor/FEditorViewportClient.h"
#include "Editor/FGizmoSettings.h"
#include "Editor/FImGuiUIScale.h"
#include "Editor/FPlayInEditorSession.h"
#include "Editor/Widget/FEditorWidget.h"
#include "Engine/FEngineShowFlags.h"
#include "FEditorDrawerManager.h"
#include "FEditorViewportLayout.h"
#include "FEditorWorkspace.h"
#include "Platform/Window.h"
#include "Serialization/JsonArchive.h"
#include <functional>
#include <memory>
#include <unordered_map>
#include <filesystem>

namespace Editor {

    /**
     * @brief 에디터 설정의 표시·직렬화·지연 저장과 테마 적용을 관리합니다.
     */
    class FEditorSettings {
    private:
        using FAction = void (Editor::FEditorSettings::*)();
        using FFloatGetter = float (Editor::FEditorSettings::*)() const;
        using FFloatSetter = void (Editor::FEditorSettings::*)(float);
        using FBoolGetter = bool (Editor::FEditorSettings::*)() const;
        using FBoolSetter = void (Editor::FEditorSettings::*)(bool);
        using FTextGetter = Core::FString (Editor::FEditorSettings::*)() const;
        using FTextSetter = void (Editor::FEditorSettings::*)(const Core::FString&);

        using FInputFloat = float ImGuiIO::*;
        using FViewportFloatGetter = float (Editor::FEditorViewportClient::*)() const;
        using FViewportFloatSetter = bool (Editor::FEditorViewportClient::*)(float);
        using FGizmoFloat = float Editor::FGizmoSettings::*;
        using FShowFlag = bool Engine::FEngineShowFlags::*;

    public:
        /**
         * @brief FEditorSettings의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Context 편집 대상과 공유 서비스를 제공하는 에디터 컨텍스트의 비소유 주소입니다.
         * @param[in] Platform 창·입력·파일 대화상자를 제공하는 플랫폼 애플리케이션입니다.
         * @param[in] PlaySession PIE 실행과 입력 상태를 제공하는 세션입니다.
         * @param[in] ViewportLayout 뷰포트 분할과 입력을 관리하는 서비스입니다.
         * @param[in] Workspace 주 탭·레벨 패널의 도킹 배치와 포커스 서비스입니다.
         * @param[in] Tabs 탭 등록과 표시 상태를 관리하는 서비스입니다.
         * @param[in] Drawers 드로어의 열기·닫기·도킹을 관리하는 서비스입니다.
         * @param[in] UIScale DPI·글꼴에 따른 에디터 UI 배율 서비스입니다.
         * @param[in] IniPath ImGui 배치 설정 파일 경로입니다.
         */
        FEditorSettings(Editor::FEditorContext* Context, Platform::FWindowsApplication* Platform, Editor::FPlayInEditorSession* PlaySession, Editor::FEditorViewportLayout& ViewportLayout, Editor::FEditorWorkspace& Workspace, Editor::FEditorTabManager& Tabs, Editor::FEditorDrawerManager& Drawers, Editor::FImGuiUIScale& UIScale, Core::FString IniPath);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FEditorSettings();

    public:
        /**
         * @brief 변경된 설정의 저장 지연 시간을 갱신하고 저장 시점에 파일로 기록합니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void Tick(float DeltaTime);

        /**
         * @brief 변경된 테마·글꼴·UI 배율을 에디터에 적용합니다.
         */
        void ApplyAppearance();

        /**
         * @brief 설정을 수정 상태로 표시하고 지연 저장 시간을 다시 설정합니다.
         */
        void MarkDirty();

        /**
         * @brief 통계 창 표시 설정을 수정 가능한 참조로 반환합니다.
         * @return 통계 표시 여부를 보관하는 설정 값입니다.
         */
        bool& GetStatisticsVisibility();

        /**
         * @brief 저장된 설정을 읽고 초기 테마와 창·패널 상태를 준비합니다.
         */
        void InitializeSettings();

        /**
         * @brief 현재 에디터 설정을 파일에 저장합니다.
         * @return 설정 저장에 성공하면 true입니다.
         */
        bool SaveSettings();

        /**
         * @brief 설정 창의 섹션을 여는 메뉴를 그립니다.
         */
        void DrawSettingsMenu();

        /**
         * @brief 선택한 설정 섹션과 저장 상태를 창에 표시합니다.
         */
        void DrawSettings();

        /**
         * @brief 지정한 섹션으로 설정 창을 열고 포커스를 요청합니다.
         * @param[in] Section 열어 표시할 설정 섹션 인덱스입니다.
         */
        void OpenSettings(Index_t Section);

    private:
        void InitializeWidgets();
        void InitializeActionWidgets();

        void InitializeInterfaceWidgets();
        void InitializeInputWidgets();

        void InitializeViewportWidgets();
        void InitializeGizmoWidgets();

        void InitializeRenderingWidgets();
        void InitializePanelWidgets();
        void InitializeProjectWidgets();

        void RegisterWidget(std::unique_ptr<Editor::FEditorWidget> Widget);
        void RegisterButton(Core::FString Label, FAction Handler);
        void RegisterTextInput(Core::FString Label, Core::FString Hint, FTextGetter Getter, FTextSetter Setter, Index_t Capacity);

        void RegisterSlider(Core::FString Label, float Minimum, float Maximum, FFloatGetter Getter, FFloatSetter Setter, Core::FString Format, float DragSpeed = 0.0f);
        void RegisterSlider(Core::FString Label, float Minimum, float Maximum, std::function<float()> Getter, std::function<void(float)> Setter, Core::FString Format, float DragSpeed = 0.0f);
        void RegisterCheckBox(Core::FString Label, FBoolGetter Getter, FBoolSetter Setter);
        void RegisterCheckBox(Core::FString Id, Core::FString Label, std::function<bool()> Getter, std::function<void(bool)> Setter);

        void RegisterInputSlider(Core::FString Label, FInputFloat Member, float Minimum, float Maximum, Core::FString Format);
        void RegisterViewportSlider(Core::FString Label, float Minimum, float Maximum, FViewportFloatGetter Getter, FViewportFloatSetter Setter, Core::FString Format, float DragSpeed);
        void RegisterGizmoSlider(Core::FString Label, FGizmoFloat Member, Editor::EGizmoSetting Setting, float Minimum, float Maximum, Core::FString Format, float DragSpeed = 0.0f);
        void RegisterStatisticsWidget(const Core::FString& Group);

        void ApplyFloatChange(const std::function<void(float)>& Setter, float Value);
        void ApplyBoolChange(const std::function<void(bool)>& Setter, bool Value);

        void SaveSettingsAction();
        void OpenResetSettingsPopup();
        void CloseSettings();
        void ConfirmResetSettings();
        void CloseSettingsPopup();

        void ReloadChangedShaders();
        void OpenShadowAtlas();
        void SaveStartupScene();

        Core::FString GetThemeFilter() const;
        void SetThemeFilter(const Core::FString& Value);
        Core::FString GetStartupScene() const;
        void SetStartupScene(const Core::FString& Value);

        bool IsAutomaticUIScale() const;
        void SetAutomaticUIScale(bool Automatic);
        bool IsManualUIScale() const;
        float GetUserUIScale() const;
        void SetUserUIScale(float Scale);
        float GetFontSize() const;
        void SetFontSize(float Size);

        bool IsKeyboardNavigationEnabled() const;
        void SetKeyboardNavigationEnabled(bool Enabled);
        bool IsTitleBarMoveOnly() const;
        void SetTitleBarMoveOnly(bool Enabled);
        float GetInputFloat(FInputFloat Member) const;
        void SetInputFloat(FInputFloat Member, float Value);

        float GetHorizontalSplit() const;
        void SetHorizontalSplit(float Value);
        float GetVerticalSplit() const;
        void SetVerticalSplit(float Value);
        float GetViewportFloat(FViewportFloatGetter Getter) const;
        void SetViewportFloat(FViewportFloatSetter Setter, float Value);

        float GetFieldOfView() const;
        void SetFieldOfView(float Value);
        float GetNearClip() const;
        void SetNearClip(float Value);
        float GetFarClip() const;
        void SetFarClip(float Value);

        float GetGizmoFloat(FGizmoFloat Member) const;
        void SetGizmoFloat(Editor::EGizmoSetting Setting, float Value);
        bool GetShowOption(FShowFlag Member) const;
        void SetShowOption(FShowFlag Member, bool Value);
        bool IsViewportCommandChecked(Editor::EViewportCommand Command) const;
        void SetViewportCommandChecked(Editor::EViewportCommand Command, bool Enabled);

        bool IsVSyncEnabled() const;
        void SetVSyncEnabled(bool Enabled);
        bool IsStatisticsWindowVisible() const;
        void SetStatisticsWindowVisible(bool Visible);
        bool IsTabVisible(const Core::FString& Id) const;
        void SetTabVisible(const Core::FString& Id, bool Visible);
        bool IsStatisticEnabled(const Core::FString& Group) const;
        void SetStatisticEnabled(const Core::FString& Group, bool Enabled);

        void DrawWidget(const char* Id);
        Editor::FEditorViewportClient& GetSettingsClient() const;

        /**
         * @brief 프로젝트의 에디터 설정 파일을 읽어 현재 상태에 반영합니다.
         */
        void LoadSettings();

        /**
         * @brief 에디터 설정을 아카이브 모드에 따라 읽거나 씁니다.
         * @param[in,out] Archive 읽기·쓰기 모드를 제공하는 아카이브입니다.
         */
        void SerializeSettings(Serialization::FJsonArchive& Archive);

        /**
         * @brief 테마·글꼴·배율 등 인터페이스 설정을 표시합니다.
         */
        void DrawInterfaceSettings();

        /**
         * @brief 선택한 뷰포트의 카메라와 표시 설정을 편집합니다.
         */
        void DrawViewportSettings();

        /**
         * @brief 기즈모 크기와 이동·회전·스케일 스냅 설정을 편집합니다.
         */
        void DrawGizmoSettings();

        /**
         * @brief 렌더링과 그림자 관련 에디터 설정을 편집합니다.
         */
        void DrawRenderingSettings();

        /**
         * @brief 패널 표시와 드로어·도킹 배치 설정을 편집합니다.
         */
        void DrawPanelSettings();

        /**
         * @brief 시작 레벨 등 프로젝트 설정을 편집합니다.
         */
        void DrawProjectSettings();

        /**
         * @brief 외부 테마 파일을 선택해 테마 카탈로그로 가져옵니다.
         */
        void ImportTheme();

        /**
         * @brief 에디터 설정을 기본값으로 되돌리고 다시 적용하도록 표시합니다.
         */
        void ResetSettings();

    private:
        std::unordered_map<Core::FString, std::unique_ptr<Editor::FEditorWidget>> mWidgets{};

        /**
         * @brief 편집 대상과 공유 서비스에 접근하는 에디터 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* mContext{};

        /**
         * @brief Win32 창·입력·파일 선택을 제공하는 플랫폼 애플리케이션입니다.
         */
        Platform::FWindowsApplication* mPlatform{};

        /**
         * @brief PIE의 월드·재생·입력 상태를 관리하는 공유 세션입니다.
         */
        Editor::FPlayInEditorSession* mPlaySession{};

        /**
         * @brief 뷰포트 분할·입력·최대화 배치를 관리하는 서비스입니다.
         */
        Editor::FEditorViewportLayout& mViewportLayout;

        /**
         * @brief 주 탭과 레벨 패널의 도킹 배치·포커스를 관리하는 서비스입니다.
         */
        Editor::FEditorWorkspace& mWorkspace;

        /**
         * @brief 위젯·주 탭을 등록하고 표시하는 탭 관리자입니다.
         */
        Editor::FEditorTabManager& mTabs;

        /**
         * @brief 드로어 등록 정보 또는 공유 드로어 관리 서비스입니다.
         */
        Editor::FEditorDrawerManager& mDrawers;

        /**
         * @brief 글꼴·DPI에 맞는 에디터 표시 배율 서비스입니다.
         */
        Editor::FImGuiUIScale& mUIScale;

        /**
         * @brief ImGui 창·도킹 배치를 저장할 INI 파일 경로입니다.
         */
        Core::FString mIniPath{};

        /**
         * @brief 사용 가능한 테마와 가져온 테마의 카탈로그입니다.
         */
        Editor::FImGuiThemeCatalog mThemes{};

        /**
         * @brief 에디터 설정 JSON 파일 경로입니다.
         */
        std::filesystem::path mSettingsPath{};

        /**
         * @brief 설정 읽기·저장 결과를 표시할 문자열입니다.
         */
        Core::FString mSettingsStatus{};

        /**
         * @brief 테마 목록 검색 입력 버퍼입니다.
         */
        Core::TStaticArray<char, 256> mThemeFilter{};

        /**
         * @brief 프로젝트를 열 때 로드할 시작 레벨 경로 입력 버퍼입니다.
         */
        Core::TStaticArray<char, 1024> mStartupScene{};

        /**
         * @brief 현재 선택한 테마 인덱스입니다.
         */
        Index_t mThemeIndex{};

        /**
         * @brief 뷰포트 설정에서 선택한 대상 슬롯 인덱스입니다.
         */
        Index_t mSettingsViewport{};

        /**
         * @brief 설정 창에서 선택한 섹션 인덱스입니다.
         */
        Index_t mSettingsSection{};

        /**
         * @brief 설정에서 선택한 글꼴 크기입니다.
         */
        float mFontSize{16.0f};

        /**
         * @brief 변경된 설정을 저장하기까지 남은 지연 시간입니다.
         */
        float mSettingsSaveDelay{};

        /**
         * @brief 설정 창의 표시 여부입니다.
         */
        bool mSettingsOpen{};

        /**
         * @brief 설정 창에 포커스를 요청할지 나타냅니다.
         */
        bool mFocusSettings{};

        /**
         * @brief 저장하지 않은 설정 변경이 있는지 나타냅니다.
         */
        bool mSettingsDirty{};

        /**
         * @brief 테마·글꼴·배율을 다시 적용해야 하는지 나타냅니다.
         */
        bool mAppearanceDirty{};

        /**
         * @brief 통계 창 표시 여부입니다.
         */
        bool mShowStatistics{};
    };

}
