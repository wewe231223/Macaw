/**
 * @file FContentBrowserWidget.h
 * @brief 콘텐츠 탐색과 에셋 생성·가져오기·이름 변경·삭제를 제공합니다.
 * @ingroup Editor
 */

#pragma once

#include "Editor/Widget/FEditorContextWidget.h"
#include "Asset/FAssetFile.h"
#include "Core/Types.h"
#include "Editor/FAssetThumbnailCache.h"
#include "Editor/FEditorAssetCatalog.h"
#include "Editor/FImGuiUIScale.h"
#include "FAssetEditorManager.h"
#include "FAssetImportController.h"
#include "FEditorWorkspace.h"
#include "Platform/Window.h"
#include "RenderCore/IRendererModule.h"
#include <filesystem>
#include <functional>
#include <optional>
#include <string_view>
#include <wrl/client.h>
#include <d3d11.h>

namespace Editor {

    /**
     * @brief 콘텐츠 탐색과 에셋 생성·가져오기·이름 변경·삭제를 제공합니다.
     */
    class FContentBrowserWidget : public Editor::FEditorContextWidget {
    private:
        /**
         * @brief 콘텐츠 경로에 적용할 이름 변경·삭제 작업입니다.
         */
        enum class EContentAction {
            /**
             * @brief 선택 콘텐츠의 이름을 변경하는 작업입니다.
             */
            Rename,

            /**
             * @brief 선택 콘텐츠를 삭제하는 작업입니다.
             */
            Delete
        };

    public:
        /**
         * @brief FContentBrowserWidget의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Context 편집 대상과 공유 서비스를 제공하는 에디터 컨텍스트의 비소유 주소입니다.
         * @param[in] Platform 창·입력·파일 대화상자를 제공하는 플랫폼 애플리케이션입니다.
         * @param[in] AssetCatalog 에셋 검색과 메타데이터를 제공하는 카탈로그입니다.
         * @param[in] Thumbnails 에셋 썸네일과 애니메이션 미리 보기 캐시입니다.
         * @param[in] UIScale DPI·글꼴에 따른 에디터 UI 배율 서비스입니다.
         * @param[in] Workspace 주 탭·레벨 패널의 도킹 배치와 포커스 서비스입니다.
         * @param[in] Imports 에셋 가져오기와 파일 드롭 서비스입니다.
         * @param[in] AssetEditors 에셋 편집기 열기와 수명을 관리하는 서비스입니다.
         */
        FContentBrowserWidget(Editor::FEditorContext* Context, Platform::FWindowsApplication* Platform, Editor::FEditorAssetCatalog& AssetCatalog, Editor::FAssetThumbnailCache& Thumbnails, Editor::FImGuiUIScale& UIScale, Editor::FEditorWorkspace& Workspace, Editor::FAssetImportController& Imports, Editor::FAssetEditorManager& AssetEditors);

        /**
         * @brief 객체를 소멸하고 소유 멤버와 연결 상태를 정리합니다.
         */
        ~FContentBrowserWidget() override;

    public:
        /**
         * @brief 예약된 콘텐츠 목록 갱신을 처리합니다.
         */
        void Tick();
        void DrawDragPreview();

        /**
         * @brief 현재 탐색 중인 콘텐츠 디렉터리를 반환합니다.
         * @return 선택된 콘텐츠 디렉터리 경로입니다.
         */
        const std::filesystem::path& GetDirectory() const;

        /**
         * @brief 현재 선택한 콘텐츠 항목의 경로를 반환합니다.
         * @return 선택 경로이며 선택 항목이 없으면 빈 경로입니다.
         */
        const std::filesystem::path& GetSelectedPath() const;

        /**
         * @brief 요청한 경로의 폴더로 이동해 해당 콘텐츠 항목을 선택합니다.
         * @param[in] Path 대상 에셋 또는 콘텐츠 경로입니다.
         */
        void Reveal(const std::filesystem::path& Path);

        /**
         * @brief 다음 갱신 시 콘텐츠 목록을 다시 읽도록 표시합니다.
         */
        void MarkRefresh();

        /**
         * @brief 콘텐츠 카탈로그 변경을 알릴 콜백을 지정합니다.
         * @param[in] Handler 에셋 카탈로그 변경을 알릴 콜백입니다.
         */
        void SetCatalogChanged(std::function<void()> Handler);

        /**
         * @brief 가져오기 결과를 반영하고 콘텐츠·카탈로그 갱신과 오류 표시를 준비합니다.
         * @param[in] Path 대상 에셋 또는 콘텐츠 경로입니다.
         * @param[in] Count 가져온 에셋 개수입니다.
         * @param[out] Error 실패 원인을 전달하는 오류 문자열입니다.
         */
        void OnImported(const std::filesystem::path& Path, Uint32 Count, const Core::FString& Error);

        /**
         * @brief 콘텐츠 종류별 아이콘의 GPU 리소스를 로드합니다.
         * @param[in] Renderer 렌더 대상과 GPU 리소스를 제공하는 렌더러입니다.
         */
        void LoadContentIcons(RenderCore::IRendererModule* Renderer);

        /**
         * @brief 프로젝트의 콘텐츠 경로와 에셋 종류·폴더 캐시를 다시 구성합니다.
         */
        void RefreshContent();

        /**
         * @brief 콘텐츠 탐색 디렉터리를 바꾸고 선택·검색 상태를 갱신합니다.
         * @param[in] Directory 탐색 또는 에셋 생성·가져오기 대상 디렉터리입니다.
         */
        void NavigateContent(const std::filesystem::path& Directory);

    private:
        /**
         * @brief 콘텐츠 탐색 트리·경로·필터·에셋 목록과 작업 팝업을 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         */
        void DrawContent(float DeltaTime) override;

        /**
         * @brief 콘텐츠 탐색 트리·경로·필터·에셋 목록과 작업 팝업을 그립니다.
         */
        void DrawContent();

        /**
         * @brief 이름 변경 또는 삭제에 사용할 실제 콘텐츠 경로를 계산합니다.
         * @param[in] Path 대상 에셋 또는 콘텐츠 경로입니다.
         * @return 콘텐츠 작업 대상 경로입니다.
         */
        std::filesystem::path GetContentActionPath(const std::filesystem::path& Path) const;

        /**
         * @brief 선택 항목의 이름 변경·삭제 정보를 준비하고 확인 팝업을 예약합니다.
         * @param[in] Action 수행할 이름 변경 또는 삭제 작업입니다.
         */
        void BeginContentAction(EContentAction Action);

        /**
         * @brief 콘텐츠 경로 작업을 막는 열린 편집기나 참조 상태를 검사합니다.
         * @param[in] Absolute 폐기 또는 경로 작업 대상의 절대 경로입니다.
         * @return 작업을 막는 이유이며 제한이 없으면 빈 문자열입니다.
         */
        Core::FString FindContentActionBlocker(const std::filesystem::path& Absolute) const;

        /**
         * @brief 이름 변경·삭제 확인과 오류·참조 목록을 표시합니다.
         */
        void DrawContentActionPopup();

        /**
         * @brief 입력한 이름을 검증하고 콘텐츠 경로를 변경합니다.
         * @return 이름 변경이 완료되면 true입니다.
         */
        bool RenameContent();

        /**
         * @brief 참조와 편집 상태를 확인하고 선택한 콘텐츠를 삭제합니다.
         * @return 삭제가 완료되면 true입니다.
         */
        bool DeleteContent();

        /**
         * @brief 선택 콘텐츠에 맞는 생성·탐색·이름 변경·삭제 메뉴를 그립니다.
         */
        void DrawContentContextMenu();

        /**
         * @brief 현재 콘텐츠 디렉터리에 새 폴더를 만듭니다.
         */
        void CreateContentFolder();

        /**
         * @brief 선택 콘텐츠 또는 현재 폴더를 Windows 탐색기에 표시합니다.
         */
        void ShowContentInExplorer() const;

        /**
         * @brief 콘텐츠 디렉터리와 하위 폴더의 탐색 트리를 그립니다.
         * @param[in] Directory 탐색 또는 에셋 생성·가져오기 대상 디렉터리입니다.
         */
        void DrawContentTree(const std::filesystem::path& Directory);

        /**
         * @brief 현재 디렉터리까지의 경로 탐색 버튼을 그립니다.
         */
        void DrawContentBreadcrumbs();

        /**
         * @brief 검색·종류 필터를 적용한 콘텐츠 항목과 미리 보기를 그립니다.
         */
        void DrawContentEntries();

        /**
         * @brief 현재 폴더에 이름이 충돌하지 않는 머티리얼 에셋을 만듭니다.
         */
        void CreateMaterial();
        void CreateMaterialIn(const std::filesystem::path& Directory);
        void CreatePhysicalMaterial();

        /**
         * @brief 현재 폴더에 새 애니메이션 그래프 에셋을 만듭니다.
         */
        void CreateAnimGraph();

        /**
         * @brief 현재 폴더에 새 블렌드 스페이스 에셋을 만듭니다.
         */
        void CreateBlendSpace();

        /**
         * @brief 현재 폴더에 새 파티클 시스템 에셋을 만듭니다.
         */
        void CreateParticleSystem();
        void CreateLuaScript();

    private:
        /**
         * @brief 편집 대상과 공유 서비스에 접근하는 에디터 컨텍스트의 비소유 주소입니다.
         */
        Editor::FEditorContext* mContext{};

        /**
         * @brief Win32 창·입력·파일 선택을 제공하는 플랫폼 애플리케이션입니다.
         */
        Platform::FWindowsApplication* mPlatform{};

        /**
         * @brief 에셋 검색과 선택에 사용하는 공유 카탈로그입니다.
         */
        Editor::FEditorAssetCatalog* mAssetCatalog{};

        /**
         * @brief 에셋 썸네일과 애니메이션 미리 보기 설정의 공유 캐시입니다.
         */
        Editor::FAssetThumbnailCache* mThumbnails{};

        /**
         * @brief 글꼴·DPI에 맞는 에디터 표시 배율 서비스입니다.
         */
        Editor::FImGuiUIScale& mUIScale;

        /**
         * @brief 주 탭과 레벨 패널의 도킹 배치·포커스를 관리하는 서비스입니다.
         */
        Editor::FEditorWorkspace& mWorkspace;

        /**
         * @brief 파일 가져오기와 드롭을 처리하는 공유 서비스입니다.
         */
        Editor::FAssetImportController& mImports;

        /**
         * @brief 열린 에셋 편집기를 관리하는 공유 서비스입니다.
         */
        Editor::FAssetEditorManager& mAssetEditors;

        /**
         * @brief 콘텐츠 탐색에서 표시할 에셋 경로 목록입니다.
         */
        Core::TArray<std::filesystem::path> mAssetPaths{};

        /**
         * @brief 다음 갱신에 콘텐츠 목록을 다시 읽을지 나타냅니다.
         */
        bool mRefreshContent{true};

        /**
         * @brief 디렉터리별 콘텐츠 하위 폴더 캐시입니다.
         */
        Core::TMap<std::filesystem::path, Core::TArray<std::filesystem::path>> mContentFolders{};

        /**
         * @brief 경로별 에셋 종류 조회 캐시입니다.
         */
        Core::TMap<std::filesystem::path, Asset::EAssetType> mAssetTypes{};

        /**
         * @brief 현재 탐색 중인 콘텐츠 디렉터리입니다.
         */
        std::filesystem::path mContentDirectory{};

        /**
         * @brief 콘텐츠 브라우저에서 선택한 파일 또는 폴더 경로입니다.
         */
        std::filesystem::path mSelectedContentPath{};

        /**
         * @brief 콘텐츠 브라우저에 표시할 최근 오류입니다.
         */
        Core::FString mContentError{};

        /**
         * @brief 탐색 트리에서 현재 폴더를 펼쳐 표시할지 나타냅니다.
         */
        bool mRevealContentDirectory{true};

        /**
         * @brief 대기 중인 콘텐츠 이름 변경 또는 삭제 작업입니다.
         */
        EContentAction mContentAction{};

        /**
         * @brief 콘텐츠 작업 확인 팝업을 열지 나타냅니다.
         */
        bool mOpenContentAction{};

        /**
         * @brief 콘텐츠 작업 대상의 파일 시스템 경로입니다.
         */
        std::filesystem::path mContentActionTarget{};

        /**
         * @brief 이름 변경 창의 새 이름 입력 버퍼입니다.
         */
        Core::TStaticArray<char, 256> mContentActionName{};

        /**
         * @brief 콘텐츠 작업 확인 창에 표시할 오류입니다.
         */
        Core::FString mContentActionError{};

        /**
         * @brief 콘텐츠 작업 대상의 참조 경로 목록입니다.
         */
        Core::TArray<std::filesystem::path> mContentActionReferences{};

        /**
         * @brief 콘텐츠 종류별 아이콘 텍스처 뷰 캐시입니다.
         */
        Core::TMap<std::string_view, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> mContentIcons{};
        Uint32 mAssetDragSourceId{};

        /**
         * @brief 콘텐츠 이름 검색 입력 버퍼입니다.
         */
        Core::TStaticArray<char, 128> mContentSearch{};

        /**
         * @brief 콘텐츠 목록에 적용할 선택적 에셋 종류 필터입니다.
         */
        std::optional<Asset::EAssetType> mContentFilter{};

        /**
         * @brief 에셋 카탈로그 변경을 알리는 콜백입니다.
         */
        std::function<void()> mCatalogChanged{};
    };

}
