/**
 * @file FAnimSequenceEditor.h
 * @brief 애니메이션 시퀀스의 복사본을 편집하고 독립 뷰어에서 재생하는 에디터입니다.
 * @ingroup Editor
 */

#pragma once

#include "FAnimSequenceTimeline.h"
#include "FMessageBox.h"
#include "Core/Types.h"
#include "CoreUObject/TObjectRef.h"
#include "Editor/FAssetPicker.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Asset/FAssetManager.h"
#include "Editor/FAssetThumbnailCache.h"
#include "Editor/FEditorAssetCatalog.h"
#include "Editor/FEditorTab.h"
#include "FAnimSequenceViewportClient.h"
#include "RenderCore/IRendererModule.h"
#include "Viewer/FAnimationViewerContent.h"
#include <filesystem>
#include <memory>
#include <optional>

namespace Editor {

    /**
     * @brief 애니메이션 시퀀스의 복사본을 편집하고 독립 뷰어에서 재생하는 에디터입니다.
     * @ingroup Editor
     */
    class FAnimSequenceEditor {
    public:

        /**
         * @brief 에셋 표시 서비스와 시퀀스 설정·뷰포트·타임라인 편집 상태를 구성합니다.
         * @param Renderer 외부에서 소유하는 렌더러 서비스입니다.
         * @param Assets 외부에서 소유하는 에셋 관리자입니다.
         * @param Catalog 외부에서 소유하는 에셋 목록입니다.
         * @param Thumbnails UI에 사용할 썸네일 캐시입니다.
         */
        FAnimSequenceEditor(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets, Editor::FEditorAssetCatalog* Catalog, Editor::FAssetThumbnailCache* Thumbnails, Editor::FMessageBox& Messages);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FAnimSequenceEditor();

    public:

        /**
         * @brief 변경 처리 확인이 필요하면 열기를 보류하고 아니면 시퀀스를 즉시 엽니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 열기 요청을 처리했으면 true이며 즉시 열기 실패 원인은 Error에 기록합니다.
         */
        bool Open(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 미리보기 메시·스켈레톤을 읽어 애니메이션 뷰어에 연결하고 설정을 보관합니다.
         * @param MeshPath 미리보기 메시의 에셋 경로입니다.
         * @param SkeletonPath 미리보기 스켈레톤의 에셋 경로입니다.
         * @return 호환되는 미리보기 에셋을 연결했으면 true입니다.
         */
        bool SetPreviewMesh(const Core::FString& MeshPath, const Core::FString& SkeletonPath);

        /**
         * @brief 호스트 주 탭에 편집기 위젯과 도킹 배치를 등록합니다.
         * @param[in] Tab 편집기를 포함할 호스트 탭 또는 등록할 외부 소유 탭입니다.
         */
        void InitializeToolkit(Editor::FEditorTab& Tab);

        /**
         * @brief 툴킷에 포함된 편집기 상태와 저장 가능 여부를 프레임별로 갱신합니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간이며 단위는 초입니다.
         * @param[in] CanSave 현재 편집 내용을 저장할 수 있는지 지정합니다.
         */
        void UpdateToolkit(float DeltaTime, bool CanSave);

        /**
         * @brief 편집기 포커스와 저장 가능 상태에 따라 단축키 명령을 처리합니다.
         * @param[in] CanSave 현재 편집 내용을 저장할 수 있는지 지정합니다.
         */
        void HandleCommands(bool CanSave);

        /**
         * @brief 저장하지 않은 애니메이션 설정을 확인할 항목을 반환합니다.
         */
        std::optional<Editor::FMessageBox::FSavePrompt> GetClosePrompt();

        /**
         * @brief 열린 에셋이 제거·교체된 경로에 해당하면 편집기를 정리합니다.
         * @param Path 제거되거나 교체된 에셋 파일 경로입니다.
         */
        void CloseRetired(const std::filesystem::path& Path);

        /**
         * @brief 편집기 창이 열려 있는지 확인합니다.
         * @return 현재 에셋 편집 창이 열려 있으면 true입니다.
         */
        bool IsOpen() const;

        /**
         * @brief 에디터 창 또는 그 하위 창이 현재 입력 포커스를 갖는지 확인합니다.
         * @return 해당 에디터에 포커스가 있으면 true입니다.
         */
        bool IsFocused() const;

        /**
         * @brief 이 에디터 창의 ImGui 식별자를 지정하여 같은 종류의 에디터를 여러 탭으로 열 수 있게 합니다.
         * @param[in] Id 세 개의 해시 기호로 시작하는 ImGui 창 식별자입니다.
         */
        void SetWindowId(Core::FString Id);

        /**
         * @brief 이 에디터 창의 ImGui 식별자를 반환합니다.
         * @return 호스트 창을 구분하는 ImGui 창 식별자 문자열입니다.
         */
        const Core::FString& GetWindowId() const;

    private:

        /**
         * @brief 시퀀스 원본과 편집용 복사본을 준비하고 타임라인·뷰어를 초기화합니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 에셋을 열었으면 true이며 실패 원인은 Error에 기록합니다.
         */
        bool OpenNow(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 편집한 시퀀스 설정을 원본과 에셋 파일에 반영합니다.
         * @return 저장에 성공하면 true입니다.
         */
        bool Save();

        /**
         * @brief 미리보기·편집 복사본을 해제하고 열린 에셋 상태를 초기화합니다.
         */
        void Close();

        /**
         * @brief 미리보기 에셋과 시퀀스 설정을 현재 뷰어에 다시 연결합니다.
         */
        void RefreshAssets();

        /**
         * @brief 시퀀스의 루트 모션과 미리보기 설정 편집 UI를 표시합니다.
         * @param CanSave 현재 상태에서 에셋 저장을 허용하는지 여부입니다.
         */
        void DrawSettings(bool CanSave);

        /**
         * @brief 검색 가능한 시퀀스 목록과 썸네일을 표시하고 열기 입력을 처리합니다.
         */
        void DrawAssetBrowser();

        /**
         * @brief 저장하지 않은 설정을 확인할 항목을 구성합니다.
         */
        Editor::FMessageBox::FSavePrompt MakeSavePrompt();

        /**
         * @brief 창 포커스와 저장 가능 상태에 따라 키보드 단축키를 처리합니다.
         * @param CanSave 현재 상태에서 에셋 저장을 허용하는지 여부입니다.
         */
        void HandleShortcuts(bool CanSave);

        /**
         * @brief 재생을 정지하고 지정 방향으로 시퀀스 한 프레임을 이동합니다.
         * @param Direction 음수는 이전, 양수는 다음 프레임입니다.
         */
        void StepFrame(Int32 Direction);

    private:

        /**
         * @brief 외부에서 소유하는 렌더러 서비스입니다.
         */
        RenderCore::IRendererModule* mRenderer{};

        /**
         * @brief 외부에서 소유하는 에셋 관리자입니다.
         */
        Asset::FAssetManager* mAssets{};

        /**
         * @brief 외부에서 소유하는 프로젝트 에셋 목록입니다.
         */
        Editor::FEditorAssetCatalog* mCatalog{};

        /**
         * @brief 외부에서 소유하는 썸네일 캐시입니다.
         */
        Editor::FAssetThumbnailCache* mThumbnails{};

        Editor::FMessageBox& mMessages;

        /**
         * @brief 에셋 선택 위젯의 상태입니다.
         */
        Editor::FAssetPicker mPicker{};

        /**
         * @brief 애니메이션 타임라인의 표시·조작 상태입니다.
         */
        Editor::FAnimSequenceTimeline mTimeline{};

        /**
         * @brief 에셋 관리자가 소유하는 원본 시퀀스입니다.
         */
        CoreUObject::TObjectRef<const Asset::UAnimSequence> mSource{};

        /**
         * @brief 원본을 보존하면서 설정을 편집할 시퀀스 복사본입니다.
         */
        std::unique_ptr<Asset::UAnimSequence> mEditing{};

        /**
         * @brief 뷰포트의 공통 뷰어가 소유하는 애니메이션 콘텐츠입니다.
         */
        Viewer::FAnimationViewerContent* mPreview{};

        /**
         * @brief 애니메이션 표시와 입력을 담당하는 뷰포트 클라이언트의 소유 포인터입니다.
         */
        std::unique_ptr<Editor::FAnimSequenceViewportClient> mViewport{};

        /**
         * @brief 현재 편집 중인 에셋 파일 경로입니다.
         */
        std::filesystem::path mPath{};

        /**
         * @brief 이 에디터 창의 ImGui 식별자입니다.
         */
        Core::FString mWindowId{"###AnimSequenceEditor"};

        /**
         * @brief 편집기를 포함하는 호스트 주 탭의 비소유 주소입니다.
         */
        Editor::FEditorTab* mHostTab{};

        /**
         * @brief 현재 툴킷에서 에셋 저장을 허용하는지 나타냅니다.
         */
        bool mToolkitCanSave{};

        /**
         * @brief 미리보기에 사용할 메시 에셋 경로입니다.
         */
        Core::FString mMeshPath{};

        /**
         * @brief 스켈레톤 에셋 경로입니다.
         */
        Core::FString mSkeletonPath{};

        /**
         * @brief 마지막 작업의 오류 메시지입니다.
         */
        Core::FString mError{};

        /**
         * @brief 루트 모션 본 이름의 UTF-8 편집 버퍼입니다.
         */
        Core::TStaticArray<char, 1024> mRootBone{};

        /**
         * @brief 애니메이션 목록의 UTF-8 검색 입력 버퍼입니다.
         */
        Core::TStaticArray<char, 256> mAnimationSearch{};

        /**
         * @brief 에디터 창이 열려 있는지 여부입니다.
         */
        bool mOpen{};

        /**
         * @brief 저장 또는 장면 동기화가 필요한 변경이 있는지 여부입니다.
         */
        bool mDirty{};

        /**
         * @brief 외부 변경으로 원본 시퀀스가 교체되거나 설정이 달라져 다시 열어야 하는지 여부입니다.
         */
        bool mSourceChanged{};

        /**
         * @brief 본 오버레이 표시 여부입니다.
         */
        bool mShowBones{};

        /**
         * @brief 미리보기 격자 표시 여부입니다.
         */
        bool mShowGrid{true};

        /**
         * @brief 와이어프레임 표시 여부입니다.
         */
        bool mWireframe{};
    };

}
