/**
 * @file FEditorUI.h
 * @brief 에디터 창, 패널과 뷰포트 표시 수명을 묶는 UI 호스트를 정의합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "Core/Math/FVector4.h"
#include "Engine/FAssetFile.h"
#include "Editor/Panel/FOutlinerPanel.h"
#include "Editor/Panel/FDetailsPanel.h"
#include "Editor/Panel/FConsolePanel.h"
#include "Editor/Panel/FPlaceActorsPanel.h"
#include "FEditorViewportLayout.h"
#include "Editor/FAssetPicker.h"
#include "Editor/FImGuiUIScale.h"
#include "FImGuiThemeCatalog.h"

#include <filesystem>
#include <optional>

namespace Serialization {
    class FJsonArchive;
}

namespace Platform {
    class FWindowsApplication;
}

namespace RenderCore {
    class IRendererModule;
}

namespace Engine {
    class UTexture2D;
    class UMaterial;
}

namespace Editor {

    class FEditorContext;
    class FEditorViewport;
    class FPlayInEditorSession;
    class FMeshViewer;
    class FEditorAssetCatalog;
    class FAssetThumbnailCache;

    /**
     * @brief 편집 모델을 참조하여 창과 패널을 표시하고 ImGui 백엔드 수명을 관리합니다.
     * @ingroup Editor
     * @note 선택, 장면 경로와 변경 상태는 FEditorContext가 소유합니다.
     */
    class FEditorUI {
    public:
        /**
         * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
         */
        ReflectTypes(FEditorUI);

    public:
        /**
         * @brief UI가 참조할 편집 경로와 뷰포트를 연결합니다.
         * @param[in,out] Context 패널이 공유하는 비소유 편집 컨텍스트입니다.
         * @param[in,out] Viewport 화면에 배치할 독립 뷰포트입니다.
         * @param[in] PlaySession 실행 제어와 조회에 사용할 PIE 세션의 비소유 주소입니다.
         */
        FEditorUI(Editor::FEditorContext* Context, Editor::FEditorViewport* Viewport, Editor::FPlayInEditorSession* PlaySession);

        /**
         * @brief UI 백엔드와 소유 표시 리소스를 정리합니다.
         */
        ~FEditorUI();

    public:
        /**
         * @brief 정적 구조체 메타데이터를 반환합니다.
         * @return 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FEditorUI);

        /**
         * @brief 독립 뷰포트를 배치할 창의 ImGui 백엔드를 준비합니다.
         * @param[in,out] Renderer ImGui 표시 리소스를 제공하는 렌더러입니다.
         * @param[in,out] Platform UI 입력을 연결할 플랫폼 창입니다.
         * @return 준비에 성공하면 true입니다.
         */
        bool Initialize(RenderCore::IRendererModule* Renderer, Platform::FWindowsApplication* Platform);

        /**
         * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
         */
        void Shutdown();

        /**
         * @brief ImGui 프레임과 입력, 패널 UI를 구성합니다.
         * @param[in] DeltaTime 이번 프레임의 초 단위 시간입니다.
         * @return UI가 초기화되어 프레임을 시작하면 true입니다.
         */
        bool BeginFrame(float DeltaTime);

        /**
         * @brief 장면과 통계를 표시하고 UI 렌더 명령을 제출합니다.
         * @param[in] DeltaTime 통계에 표시할 프레임 시간입니다.
         * @note BeginFrame() 이후 엔진 갱신과 월드 렌더 동기화를 수행한 뒤 호출합니다.
         */
        void EndFrame(float DeltaTime);

        /**
         * @brief 에디터에서 프로그램 종료를 요청했는지 검사합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool WantsExit() const;

        /**
         * @brief 가장 최근 입력을 받은 유효한 뷰포트를 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Editor::FEditorViewport* GetActiveViewport() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수의 접근 권한입니다.
         */
        ReflectMembers();

        /**
         * @brief 프로젝트의 Editor/Icon 폴더에 없는 언리얼 아이콘과 출처 파일을 배포 템플릿에서 복사합니다.
         * @note 기존 파일을 유지하며 원본 탐색이나 복사에 실패하면 에디터 로그에 원인을 기록합니다.
         */
        void InstallEditorIcons();

        /**
         * @brief 장면 파일과 패널 표시 메뉴를 그립니다.
         */
        void DrawMenu();

        /**
         * @brief 내보낼 폴더를 고르고 저장하지 않은 장면을 확인한 뒤 현재 프로젝트를 독립 실행 게임으로 내보냅니다.
         * @note 게임 런타임은 에디터 실행 파일 옆의 GameRuntime 폴더에서 찾습니다. 결과는 로그와 메시지 상자로 알립니다.
         */
        void ExportGame();

        /**
         * @brief 기본값과 테마 카탈로그를 준비하고 프로젝트의 저장된 에디터 환경 설정을 복원합니다.
         */
        void InitializeSettings();
        /**
         * @brief 프로젝트의 버전 1 환경 설정을 읽고 실패 원인을 상태 문자열과 에디터 로그에 기록합니다.
         */
        void LoadSettings();
        /**
         * @brief 환경 설정을 임시 파일에 기록한 뒤 교체하고 ImGui 배치도 저장합니다.
         * @return 모든 설정을 저장하면 true이며 파일 처리에 실패하면 false입니다.
         */
        bool SaveSettings();
        /**
         * @brief 테마·폰트·입력·패널과 네 뷰포트의 환경 설정을 아카이브로 저장하거나 복원합니다.
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         */
        void SerializeSettings(Serialization::FJsonArchive& Archive);
        /**
         * @brief 상단 메뉴에 테마·UI 배율·VSync와 설정 창 열기 항목을 표시합니다.
         */
        void DrawSettingsMenu();
        /**
         * @brief 선택한 설정 분류의 편집 UI와 저장·초기화 상태를 표시합니다.
         */
        void DrawSettings();
        /**
         * @brief 테마 검색·가져오기와 기본 폰트 크기 등 인터페이스 설정을 편집합니다.
         */
        void DrawInterfaceSettings();
        /**
         * @brief 선택한 뷰포트의 카메라·컬러 모드·버퍼 시각화·ShowFlag를 편집합니다.
         */
        void DrawViewportSettings();
        /**
         * @brief 선택한 뷰포트의 변환 도구·좌표계·스냅 간격과 기즈모 크기를 편집합니다.
         */
        void DrawGizmoSettings();
        /**
         * @brief VSync와 뷰포트별 렌더링 관련 표시 설정을 편집합니다.
         */
        void DrawRenderingSettings();
        /**
         * @brief 에디터 패널과 선택한 뷰포트의 통계 그룹 표시 설정을 편집합니다.
         */
        void DrawPanelSettings();
        /**
         * @brief 프로젝트 경로와 시작 장면을 표시하고 시작 장면 설정을 저장합니다.
         */
        void DrawProjectSettings();
        /**
         * @brief 파일 선택으로 ImThemes TOML을 가져오고 프로젝트 Saved/Themes에 보관합니다.
         */
        void ImportTheme();
        /**
         * @brief 에디터 환경 설정·패널·뷰포트·입력·배율을 기본값으로 되돌립니다.
         */
        void ResetSettings();

        /**
         * @brief 그림자 아틀라스 미리보기와 타일별 광원 정보를 그립니다.
         */
        void DrawShadowAtlas();

        /**
         * @brief 프로젝트·장면 작업과 편집 도구를 제공하는 툴바를 그립니다.
         */
        void DrawEditorToolbar();

        /**
         * @brief PIE 시작·일시정지·단계 실행·종료 버튼을 그립니다.
         */
        void DrawPlayControls();

        /**
         * @brief 프로젝트 에셋 목록과 생성·가져오기 UI를 그립니다.
         */
        void DrawContent();

        /**
         * @brief 선택한 텍스처의 미리보기 창을 그립니다.
         */
        void DrawTexturePreview();

        /**
         * @brief 현재 Content 폴더와 에셋 종류 목록을 다시 읽습니다.
         */
        void RefreshContent();

        /**
         * @brief 콘텐츠 브라우저의 현재 폴더와 선택을 변경합니다.
         * @param[in] Directory 탐색할 Content 디렉터리입니다.
         */
        void NavigateContent(const std::filesystem::path& Directory);

        /**
         * @brief Content 하위 폴더 트리를 재귀적으로 표시합니다.
         * @param[in] Directory 탐색할 Content 디렉터리입니다.
         */
        void DrawContentTree(const std::filesystem::path& Directory);

        /**
         * @brief 현재 콘텐츠 폴더의 상위 경로 탐색 항목을 표시합니다.
         */
        void DrawContentBreadcrumbs();

        /**
         * @brief 현재 폴더의 하위 폴더·에셋 항목과 드래그 원본을 표시합니다.
         */
        void DrawContentEntries();

        /**
         * @brief 종류에 맞게 선택 에셋을 열거나 액터 배치에 사용합니다.
         * @param[in] Path Content 기준 에셋 경로입니다.
         * @param[in] Type 에셋 종류입니다.
         */
        void OpenContentAsset(const std::filesystem::path& Path, Engine::EAssetType Type);

        /**
         * @brief 기본 설정으로 새 머티리얼 에셋을 만듭니다.
         */
        void CreateMaterial();

        /**
         * @brief 편집 중인 머티리얼의 프로퍼티와 저장 상태를 표시합니다.
         */
        void DrawMaterialEditor();

        /**
         * @brief 편집 사본을 프로젝트 머티리얼에 저장합니다.
         * @return 성공하면 true, 실패하면 false입니다.
         */
        bool SaveMaterial();

        /**
         * @brief 미저장 머티리얼을 닫기 전 사용자 선택을 처리합니다.
         * @return 성공하면 true, 실패하면 false입니다.
         */
        bool ConfirmMaterialChange();

        /**
         * @brief 머티리얼 편집 사본의 텍스처 에셋을 검증하여 교체합니다.
         * @param[in] Path Content 기준 텍스처 에셋 경로이며 비우면 텍스처를 제거합니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetMaterialTexture(const Core::FString& Path);

        /**
         * @brief 머티리얼 편집 사본의 셰이더 경로를 검증하여 교체합니다.
         * @param[in] Path Content 기준 셰이더 경로입니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetMaterialShader(const Core::FString& Path);

        /**
         * @brief 파일 대화상자에서 고른 메시 또는 이미지를 프로젝트 에셋으로 가져옵니다.
         */
        void ImportAsset();

        /**
         * @brief 원본 파일 목록을 대상 콘텐츠 디렉터리에 가져오고 에셋 목록과 결과 로그를 갱신합니다.
         * @param[in] Sources 가져올 원본 파일 경로 목록입니다.
         * @param[in] Directory 가져온 에셋을 저장할 Content 폴더입니다.
         */
        void ImportFiles(const Core::TArray<std::filesystem::path>& Sources, const std::filesystem::path& Directory);
        /**
         * @brief Windows 드롭 핸들에서 경로와 대상 영역을 수집하여 가져오기를 대기열에 넣고 핸들을 해제합니다.
         * @param[in] Handle 처리 후 DragFinish로 해제할 Windows HDROP 핸들입니다.
         */
        void QueueFileDrop(void* Handle);
        /**
         * @brief 대기 중인 파일 드롭 묶음을 꺼내 각 대상 디렉터리로 가져옵니다.
         */
        void ImportDroppedFiles();
        /**
         * @brief 콘텐츠 영역을 현재 ImGui 클립 사각형으로 잘라 파일 드롭 대상으로 등록합니다.
         * @param[in] Directory 드롭한 파일을 가져올 Content 폴더입니다.
         * @param[in] Bounds 드롭 영역의 화면 좌표 최소 XY와 최대 ZW입니다.
         */
        void RegisterContentDropTarget(const std::filesystem::path& Directory, const Core::FVector4& Bounds);

        /**
         * @brief 프로젝트 파일 대화상자를 열고 선택한 경로의 전환을 요청합니다.
         * @param[in] Create true이면 새 프로젝트를 생성하고 false이면 기존 프로젝트를 엽니다.
         */
        void ChooseProject(bool Create);

        /**
         * @brief 저장하지 않은 장면 변경을 확인하고 전환을 계속할 수 있는지 판정합니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool ConfirmSceneChange();

        /**
         * @brief 에디터 패널의 초기 ImGui 도킹 레이아웃을 구성합니다.
         */
        void BuildLayout();

        /**
         * @brief 장면 로드 또는 저장용 Windows 파일 대화상자를 표시합니다.
         * @param[in] Save 저장용 대화상자를 열지 지정합니다.
         * @return 선택한 파일 경로이며, 취소하면 빈 경로입니다.
         */
        std::filesystem::path ChooseFile(bool Save);

    private:
        /**
         * @brief 패널이 공유하는 편집 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* mContext{};
        ReflectProperty(mContext, "Context", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief UI에 배치할 독립 뷰포트의 비소유 주소입니다.
         */
        Editor::FEditorViewport* mViewport{};
        ReflectProperty(mViewport, "Viewport", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief PIE 제어와 조회를 위한 세션의 비소유 주소입니다.
         */
        Editor::FPlayInEditorSession* mPlaySession{};
        ReflectProperty(mPlaySession, "PlaySession", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 단일·4분할 뷰포트 배치 관리자입니다.
         */
        Editor::FEditorViewportLayout mViewportLayout;
        ReflectProperty(mViewportLayout, "ViewportLayout", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Windows 창과 메시지 처리 객체입니다.
         */
        Platform::FWindowsApplication* mPlatform{};
        ReflectProperty(mPlatform, "Platform", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 액터·컴포넌트 계층 패널입니다.
         */
        Editor::FPlaceActorsPanel mPlaceActorsPanel;
        ReflectProperty(mPlaceActorsPanel, "PlaceActorsPanel", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 액터 트리를 표시하는 아웃라이너 패널의 소유 객체입니다.
         */
        Editor::FOutlinerPanel mOutlinerPanel;
        ReflectProperty(mOutlinerPanel, "OutlinerPanel", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 선택 객체의 프로퍼티 패널입니다.
         */
        Editor::FDetailsPanel mDetailsPanel;
        ReflectProperty(mDetailsPanel, "DetailsPanel", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 로그와 명령 입력 패널입니다.
         */
        Editor::FConsolePanel mConsolePanel;
        ReflectProperty(mConsolePanel, "ConsolePanel", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief ImGui 설정 파일 경로입니다.
         */
        Core::FString mIniPath{};
        ReflectProperty(mIniPath, "IniPath", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 에디터 ImGui 스타일의 화면 배율 관리자입니다.
         */
        Editor::FImGuiUIScale mUIScale{};
        ReflectProperty(mUIScale, "UIScale", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프로젝트 설정에서 선택·검색할 기본 테마와 가져온 ImThemes 카탈로그입니다.
         */
        Editor::FImGuiThemeCatalog mThemes{};
        ReflectProperty(mThemes, "Themes", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 프로젝트 Saved 폴더의 EditorSettings.json 경로입니다.
         */
        std::filesystem::path mSettingsPath{};
        ReflectProperty(mSettingsPath, "SettingsPath", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 설정 저장·복원·테마 가져오기의 최근 결과 또는 오류 메시지입니다.
         */
        Core::FString mSettingsStatus{};
        ReflectProperty(mSettingsStatus, "SettingsStatus", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 테마 이름과 작성자를 검색하는 UTF-8 입력 버퍼입니다.
         */
        std::array<char, 256> mThemeFilter{};
        ReflectProperty(mThemeFilter, "ThemeFilter", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 프로젝트 시작 장면의 Content 상대 경로를 편집하는 입력 버퍼입니다.
         */
        std::array<char, 1024> mStartupScene{};
        ReflectProperty(mStartupScene, "StartupScene", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 테마 카탈로그에서 현재 선택한 항목의 0 기반 인덱스입니다.
         */
        Core::Uint32 mThemeIndex{};
        ReflectProperty(mThemeIndex, "ThemeIndex", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 뷰포트별 설정을 편집할 대상의 0 기반 인덱스입니다.
         */
        Core::Uint32 mSettingsViewport{};
        ReflectProperty(mSettingsViewport, "SettingsViewport", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 설정 창에서 선택한 인터페이스·뷰포트·렌더링 등 분류의 인덱스입니다.
         */
        int mSettingsSection{};
        ReflectProperty(mSettingsSection, "SettingsSection", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief DPI와 사용자 배율을 적용하기 전의 기본 폰트 크기이며 10~32 범위입니다.
         */
        float mFontSize{16.0f};
        ReflectProperty(mFontSize, "FontSize", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 설정 변경 후 자동 저장을 지연하는 남은 초 단위 시간입니다.
         */
        float mSettingsSaveDelay{};
        ReflectProperty(mSettingsSaveDelay, "SettingsSaveDelay", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 설정 창의 표시 여부입니다.
         */
        bool mSettingsOpen{};
        ReflectProperty(mSettingsOpen, "SettingsOpen", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 다음 설정 창 표시 시 포커스를 요청할지 나타냅니다.
         */
        bool mFocusSettings{};
        ReflectProperty(mFocusSettings, "FocusSettings", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 프로젝트 설정 파일에 아직 저장하지 않은 환경 설정 변경 여부입니다.
         */
        bool mSettingsDirty{};
        ReflectProperty(mSettingsDirty, "SettingsDirty", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 테마 또는 폰트가 바뀌어 기준 ImGui 스타일을 다시 적용해야 하는지 나타냅니다.
         */
        bool mAppearanceDirty{};
        ReflectProperty(mAppearanceDirty, "AppearanceDirty", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 에디터 초기화 완료 여부입니다.
         */
        bool mInitialized{};
        ReflectProperty(mInitialized, "Initialized", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 그림자 아틀라스 미리보기의 확대 배율입니다.
         */
        float mShadowAtlasZoom{1.0f};
        ReflectProperty(mShadowAtlasZoom, "ShadowAtlasZoom", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief ImGui 컨텍스트 생성 여부입니다.
         */
        bool mContextCreated{};
        ReflectProperty(mContextCreated, "ContextCreated", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief ImGui Win32 백엔드 초기화 여부입니다.
         */
        bool mWin32Ready{};
        ReflectProperty(mWin32Ready, "Win32Ready", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief ImGui Direct3D 11 백엔드 초기화 여부입니다.
         */
        bool mDxReady{};
        ReflectProperty(mDxReady, "DxReady", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프로그램 종료 요청 여부입니다.
         */
        bool mShowStatistics{};
        ReflectProperty(mShowStatistics, "ShowStatistics", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 에디터 종료 요청 여부입니다.
         */
        bool mExit{};
        ReflectProperty(mExit, "Exit", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 콘텐츠 패널에 표시할 프로젝트 상대 에셋 경로 목록입니다.
         */
        Core::TArray<std::filesystem::path> mAssetPaths{};
        ReflectProperty(mAssetPaths, "AssetPaths", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 다음 콘텐츠 패널 갱신에서 에셋 목록을 다시 읽을지 나타냅니다.
         */
        bool mRefreshContent{true};
        ReflectProperty(mRefreshContent, "RefreshContent", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 미리보기에 사용하는 에셋 관리자 소유 텍스처의 비소유 주소입니다.
         */
        Engine::UTexture2D* mPreviewTexture{};
        ReflectProperty(mPreviewTexture, "PreviewTexture", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 미리볼 텍스처의 Content 상대 경로입니다.
         */
        std::filesystem::path mPreviewTexturePath{};
        ReflectProperty(mPreviewTexturePath, "PreviewTexturePath", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 다음 그리기에서 텍스처 미리보기 창에 초점을 줄지 나타냅니다.
         */
        bool mFocusTexturePreview{};
        ReflectProperty(mFocusTexturePreview, "FocusTexturePreview", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 콘텐츠 트리에 표시할 폴더 목록입니다.
         */
        Core::TMap<std::filesystem::path, Core::TArray<std::filesystem::path>> mContentFolders{};
        ReflectProperty(mContentFolders, "ContentFolders", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 콘텐츠 파일별 에셋 종류입니다.
         */
        Core::TMap<std::filesystem::path, Engine::EAssetType> mAssetTypes{};
        ReflectProperty(mAssetTypes, "AssetTypes", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 콘텐츠 브라우저 디렉터리입니다.
         */
        std::filesystem::path mContentDirectory{};
        ReflectProperty(mContentDirectory, "ContentDirectory", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 이번 프레임에 등록한 콘텐츠 드롭 영역별 대상 디렉터리입니다.
         */
        Core::TArray<std::filesystem::path> mContentDropDirectories{};
        ReflectProperty(mContentDropDirectories, "ContentDropDirectories", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 드롭 대상별로 클리핑한 화면 사각형의 최소 XY와 최대 ZW입니다.
         */
        Core::TArray<Core::FVector4> mContentDropBounds{};
        ReflectProperty(mContentDropBounds, "ContentDropBounds", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Windows 파일 드롭에서 수집하여 프레임 경계에서 가져올 원본 경로 묶음입니다.
         */
        Core::TArray<Core::TArray<std::filesystem::path>> mDroppedFiles{};
        ReflectProperty(mDroppedFiles, "DroppedFiles", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 대기 중인 각 파일 드롭 묶음에 대응하는 콘텐츠 대상 디렉터리입니다.
         */
        Core::TArray<std::filesystem::path> mDropDirectories{};
        ReflectProperty(mDropDirectories, "DropDirectories", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 선택한 콘텐츠 에셋 경로입니다.
         */
        std::filesystem::path mSelectedContentPath{};
        ReflectProperty(mSelectedContentPath, "SelectedContentPath", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 콘텐츠 검색 또는 에셋 작업의 오류입니다.
         */
        Core::FString mContentError{};
        ReflectProperty(mContentError, "ContentError", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 선택 에셋의 폴더를 트리에서 펼칠지 여부입니다.
         */
        bool mRevealContentDirectory{true};
        ReflectProperty(mRevealContentDirectory, "RevealContentDirectory", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 편집 중인 머티리얼의 독립 사본입니다.
         */
        Engine::UMaterial* mEditingMaterial{};
        ReflectProperty(mEditingMaterial, "EditingMaterial", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Content 기준 머티리얼 파일 경로입니다.
         */
        std::filesystem::path mMaterialPath{};
        ReflectProperty(mMaterialPath, "MaterialPath", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 머티리얼 편집 또는 저장 오류입니다.
         */
        Core::FString mMaterialError{};
        ReflectProperty(mMaterialError, "MaterialError", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 미저장 머티리얼 변경 여부입니다.
         */
        bool mMaterialDirty{};
        ReflectProperty(mMaterialDirty, "MaterialDirty", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 편집 월드와 분리한 정적 메시 미리보기 창입니다.
         */
        std::unique_ptr<Editor::FMeshViewer> mMeshViewer{};
        ReflectProperty(mMeshViewer, "MeshViewer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 브라우저와 Details가 공유하는 실제 에셋 목록입니다.
         */
        std::unique_ptr<Editor::FEditorAssetCatalog> mAssetCatalog{};
        ReflectProperty(mAssetCatalog, "AssetCatalog", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 재질·메시 썸네일과 재질 편집 미리보기의 공유 캐시입니다.
         */
        std::unique_ptr<Editor::FAssetThumbnailCache> mThumbnails{};
        ReflectProperty(mThumbnails, "Thumbnails", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 재질 에디터의 텍스처 선택기입니다.
         */
        Editor::FAssetPicker mAssetPicker{};
        ReflectProperty(mAssetPicker, "AssetPicker", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 에셋 브라우저 검색 문자열입니다.
         */
        std::array<char, 128> mContentSearch{};
        ReflectProperty(mContentSearch, "ContentSearch", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 선택한 에셋 종류 필터입니다.
         */
        std::optional<Engine::EAssetType> mContentFilter{};
        ReflectProperty(mContentFilter, "ContentFilter", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

    };

}
