/**
 * @file FEditorUI.h
 * @brief 에디터 창, 패널과 뷰포트 표시 수명을 묶는 UI 호스트를 정의합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "Engine/FAssetFile.h"
#include "Editor/Panel/FOutlinerPanel.h"
#include "Editor/Panel/FDetailsPanel.h"
#include "Editor/Panel/FConsolePanel.h"
#include "FEditorViewportLayout.h"
#include "Editor/FAssetPicker.h"

#include <filesystem>
#include <optional>

class FEditorContext;
class FEditorViewport;
class FWindowsApplication;
class IRendererModule;
class FPlayInEditorSession;
class UTexture2D;
class UMaterial;
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
     */
    FEditorUI(FEditorContext* Context, FEditorViewport* Viewport, FPlayInEditorSession* PlaySession);
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
    bool Initialize(IRendererModule* Renderer, FWindowsApplication* Platform);
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
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    FEditorViewport* GetActiveViewport() const;

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
     * @brief 프로젝트·장면 작업과 편집 도구를 제공하는 툴바를 그립니다.
     */
    void DrawEditorToolbar();
    /**
     * @brief PIE 시작·일시정지·단계 실행·종료 버튼을 그립니다.
     */
    void DrawPlayControls();
    /**
     * @brief 프로젝트 에셋 목록과 메시 생성·텍스처 미리보기 UI를 그립니다.
     */
    void DrawContent();
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
     * @param[in] Path Content 상대 에셋 경로 또는 작업할 파일 경로입니다.
     * @param[in] Type 선택할 뷰 방향 또는 에셋 종류입니다.
     */
    void OpenContentAsset(const std::filesystem::path& Path, EAssetType Type);
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
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SaveMaterial();
    /**
     * @brief 미저장 머티리얼을 닫기 전 사용자 선택을 처리합니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool ConfirmMaterialChange();
    /**
     * @brief 머티리얼 편집 사본의 텍스처 에셋을 검증하여 교체합니다.
     * @param[in] Path Content 상대 에셋 경로 또는 작업할 파일 경로입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetMaterialTexture(const FString& Path);
    /**
     * @brief 머티리얼 편집 사본의 셰이더 경로를 검증하여 교체합니다.
     * @param[in] Path Content 상대 에셋 경로 또는 작업할 파일 경로입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetMaterialShader(const FString& Path);
    /**
     * @brief 파일 대화상자에서 고른 메시 또는 이미지를 프로젝트 에셋으로 가져옵니다.
     * @param[in] Texture true이면 이미지를, false이면 메시를 가져옵니다.
     */
    void ImportAsset(bool Texture);
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
     * @return 연산을 적용한 새 값입니다.
     */
    std::filesystem::path ChooseFile(bool Save);

private:
    /**
     * @brief 패널이 공유하는 편집 컨텍스트의 비소유 주소입니다.
     */
    FEditorContext* mContext{};
    ReflectProperty(mContext, "Context", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief UI에 배치할 독립 뷰포트의 비소유 주소입니다.
     */
    FEditorViewport* mViewport{};
    ReflectProperty(mViewport, "Viewport", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief PIE 제어와 조회를 위한 세션의 비소유 주소입니다.
     */
    FPlayInEditorSession* mPlaySession{};
    ReflectProperty(mPlaySession, "PlaySession", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 단일·4분할 뷰포트 배치 관리자입니다.
     */
    FEditorViewportLayout mViewportLayout;
    ReflectProperty(mViewportLayout, "ViewportLayout", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief Windows 창과 메시지 처리 객체입니다.
     */
    FWindowsApplication* mPlatform{};
    ReflectProperty(mPlatform, "Platform", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 액터·컴포넌트 계층 패널입니다.
     */
    FOutlinerPanel mOutlinerPanel;
    ReflectProperty(mOutlinerPanel, "OutlinerPanel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 선택 객체의 프로퍼티 패널입니다.
     */
    FDetailsPanel mDetailsPanel;
    ReflectProperty(mDetailsPanel, "DetailsPanel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 로그와 명령 입력 패널입니다.
     */
    FConsolePanel mConsolePanel;
    ReflectProperty(mConsolePanel, "ConsolePanel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief ImGui 설정 파일 경로입니다.
     */
    FString mIniPath{};
    ReflectProperty(mIniPath, "IniPath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 에디터 초기화 완료 여부입니다.
     */
    bool mInitialized{};
    ReflectProperty(mInitialized, "Initialized", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief ImGui 컨텍스트 생성 여부입니다.
     */
    bool mContextCreated{};
    ReflectProperty(mContextCreated, "ContextCreated", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief ImGui Win32 백엔드 초기화 여부입니다.
     */
    bool mWin32Ready{};
    ReflectProperty(mWin32Ready, "Win32Ready", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief ImGui Direct3D 11 백엔드 초기화 여부입니다.
     */
    bool mDxReady{};
    ReflectProperty(mDxReady, "DxReady", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 프로그램 종료 요청 여부입니다.
     */
    bool mExit{};
    ReflectProperty(mExit, "Exit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 콘텐츠 패널에 표시할 프로젝트 상대 에셋 경로 목록입니다.
     */
    TArray<std::filesystem::path> mAssetPaths{};
    ReflectProperty(mAssetPaths, "AssetPaths", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 다음 콘텐츠 패널 갱신에서 에셋 목록을 다시 읽을지 나타냅니다.
     */
    bool mRefreshContent{true};
    ReflectProperty(mRefreshContent, "RefreshContent", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 미리보기에 사용하는 에셋 관리자 소유 텍스처의 비소유 주소입니다.
     */
    UTexture2D* mPreviewTexture{};
    ReflectProperty(mPreviewTexture, "PreviewTexture", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 콘텐츠 트리에 표시할 폴더 목록입니다.
     */
    TMap<std::filesystem::path, TArray<std::filesystem::path>> mContentFolders{};
    ReflectProperty(mContentFolders, "ContentFolders", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 콘텐츠 파일별 에셋 종류입니다.
     */
    TMap<std::filesystem::path, EAssetType> mAssetTypes{};
    ReflectProperty(mAssetTypes, "AssetTypes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 현재 콘텐츠 브라우저 디렉터리입니다.
     */
    std::filesystem::path mContentDirectory{};
    ReflectProperty(mContentDirectory, "ContentDirectory", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 선택한 콘텐츠 에셋 경로입니다.
     */
    std::filesystem::path mSelectedContentPath{};
    ReflectProperty(mSelectedContentPath, "SelectedContentPath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 콘텐츠 검색 또는 에셋 작업의 오류입니다.
     */
    FString mContentError{};
    ReflectProperty(mContentError, "ContentError", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 선택 에셋의 폴더를 트리에서 펼칠지 여부입니다.
     */
    bool mRevealContentDirectory{true};
    ReflectProperty(mRevealContentDirectory, "RevealContentDirectory", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 편집 중인 머티리얼의 독립 사본입니다.
     */
    UMaterial* mEditingMaterial{};
    ReflectProperty(mEditingMaterial, "EditingMaterial", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief Content 기준 머티리얼 파일 경로입니다.
     */
    std::filesystem::path mMaterialPath{};
    ReflectProperty(mMaterialPath, "MaterialPath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 머티리얼 편집 또는 저장 오류입니다.
     */
    FString mMaterialError{};
    ReflectProperty(mMaterialError, "MaterialError", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 미저장 머티리얼 변경 여부입니다.
     */
    bool mMaterialDirty{};
    ReflectProperty(mMaterialDirty, "MaterialDirty", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 편집 월드와 분리한 정적 메시 미리보기 창입니다. */
    std::unique_ptr<FMeshViewer> mMeshViewer{};
    ReflectProperty(mMeshViewer, "MeshViewer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 브라우저와 Details가 공유하는 논리 에셋 목록입니다. */
    std::unique_ptr<FEditorAssetCatalog> mAssetCatalog{};
    ReflectProperty(mAssetCatalog, "AssetCatalog", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 재질·메시 썸네일과 재질 편집 미리보기의 공유 캐시입니다. */
    std::unique_ptr<FAssetThumbnailCache> mThumbnails{};
    ReflectProperty(mThumbnails, "Thumbnails", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 물리 경로에 대응하는 표시 경로입니다. */
    TMap<std::filesystem::path, std::filesystem::path> mAssetDisplayPaths{};
    ReflectProperty(mAssetDisplayPaths, "AssetDisplayPaths", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 재질 에디터의 텍스처 선택기입니다. */
    FAssetPicker mAssetPicker{};
    ReflectProperty(mAssetPicker, "AssetPicker", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 에셋 브라우저 검색 문자열입니다. */
    std::array<char, 128> mContentSearch{};
    ReflectProperty(mContentSearch, "ContentSearch", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 선택한 에셋 종류 필터입니다. */
    std::optional<EAssetType> mContentFilter{};
    ReflectProperty(mContentFilter, "ContentFilter", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

};
