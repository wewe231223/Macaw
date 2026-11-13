/**
 * @file FAnimGraphEditor.h
 * @brief 애니메이션 그래프 에셋의 노드를 배치·연결하고 변수와 파라미터를 편집하는 창입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Editor/FAssetPicker.h"

#include <array>
#include <filesystem>
#include <memory>

namespace ax::NodeEditor {
    struct EditorContext;
}

namespace Asset {
    class FAssetManager;
    class UAnimGraph;
}

namespace Editor {

    class FAssetThumbnailCache;
    class FEditorAssetCatalog;

    /**
     * @brief 언리얼 AnimGraph 편집기처럼 노드를 배치하고 포즈 핀을 연결해 그래프 에셋을 만듭니다.
     * @details 캔버스 빈 곳을 우클릭하면 노드를 추가하고, 출력 핀(Pose)을 다른 노드의 입력 핀으로 끌어 연결하며, 선택 후 Delete로 지웁니다.
     * 입력 핀 하나에는 링크가 하나만 연결되고, 순환 연결은 거부합니다.
     * 편집은 사본에 하고, 파일 저장에 성공하면 캐시된 원본에 적용합니다.
     */
    class FAnimGraphEditor {
    private:

        /**
         * @brief 애니메이션 그래프 에디터의 포즈·상태 머신·전이 조건 보기 종류입니다.
         * @ingroup Editor
         */
        enum class EGraphView {

            /**
             * @brief 포즈·값 연결 그래프를 표시합니다.
             */
            Pose,

            /**
             * @brief 상태와 전이 연결을 표시합니다.
             */
            Machine,

            /**
             * @brief 전이의 Bool 조건 그래프를 표시합니다.
             */
            Rule
        };

        /**
         * @brief 중첩 그래프 탐색 시 복원할 그래프, 보기 종류와 선택 상태입니다.
         * @ingroup Editor
         */
        struct FGraphView {

            /**
             * @brief 현재 보기에 대응하는 비소유 포즈 그래프 포인터입니다.
             */
            Asset::UAnimGraph* mGraph{};

            /**
             * @brief 포즈, 상태 머신, 전이 조건 중 현재 보기 종류입니다.
             */
            EGraphView mMode{};

            /**
             * @brief 현재 보기가 속한 상태 머신 노드 ID입니다.
             */
            Core::Int32 mMachine{-1};

            /**
             * @brief 현재 보기에서 편집할 전이 ID입니다.
             */
            Core::Int32 mTransition{-1};

            /**
             * @brief 현재 보기에서 선택한 상태 또는 전이 ID입니다.
             */
            Core::Int32 mSelection{-2};

            /**
             * @brief UI에 표시할 이름입니다.
             */
            Core::FString mLabel{"AnimGraph"};

            /**
             * @brief 중첩 그래프별 노드 편집기 상태를 구분하는 탐색 키입니다.
             */
            Core::FString mKey{"Root"};
        };

    public:

        /**
         * @brief 에셋 관리자와 선택기 서비스를 연결하고 노드 에디터 컨텍스트를 만듭니다.
         * @param[in] Assets 프로젝트 에셋 관리자이며 창보다 오래 살아 있어야 합니다.
         * @param[in] Catalog 에셋 선택기에 쓸 목록입니다.
         * @param[in] Thumbnails 에셋 선택기에 쓸 썸네일 캐시입니다.
         */
        FAnimGraphEditor(Asset::FAssetManager* Assets, Editor::FEditorAssetCatalog* Catalog, Editor::FAssetThumbnailCache* Thumbnails);

        /**
         * @brief 노드 에디터 컨텍스트를 해제합니다.
         */
        ~FAnimGraphEditor();

        /**
         * @brief 소유 리소스와 편집 상태의 복사 생성을 금지합니다.
         */
        FAnimGraphEditor(const Editor::FAnimGraphEditor&) = delete;

        /**
         * @brief 소유 리소스와 편집 상태의 복사 대입을 금지합니다.
         */
        Editor::FAnimGraphEditor& operator=(const Editor::FAnimGraphEditor&) = delete;

    public:

        /**
         * @brief 그래프를 열어 편집 사본을 만듭니다. 저장하지 않은 변경은 버립니다.
         * @param[in] Path Content 기준 경로입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 열었으면 true입니다.
         */
        bool Open(const std::filesystem::path& Path, Core::FString& Error);

        /**
         * @brief 열려 있으면 창을 그립니다.
         */
        void Draw();

        /**
         * @brief 열린 에셋이 지정한 경로나 그 아래에 있으면 닫습니다.
         * @param[in] Path 절대 경로나 Content 기준 경로입니다.
         */
        void CloseRetired(const std::filesystem::path& Path);

        /**
         * @brief 에디터 창 또는 그 하위 창이 현재 입력 포커스를 갖는지 확인합니다.
         * @return 해당 에디터에 포커스가 있으면 true입니다.
         */
        bool IsFocused() const;

        /**
         * @brief 포즈 그래프 노드에 연결된 상태 머신 보기를 엽니다.
         * @param NodeId 원본 그래프 노드 ID입니다.
         * @return 상태 머신 데이터를 찾았으면 true입니다.
         */
        bool OpenStateMachine(Core::Int32 NodeId);

        /**
         * @brief 선택한 상태의 중첩 포즈 그래프를 엽니다.
         * @param StateId 열거나 조회할 상태 ID입니다.
         * @return 상태와 포즈 그래프가 존재하면 true입니다.
         */
        bool OpenState(Core::Int32 StateId);

        /**
         * @brief 선택한 전이의 Bool 조건 그래프를 엽니다.
         * @param TransitionId 열거나 조회할 전이 ID입니다.
         * @return 전이 데이터를 찾았으면 true입니다.
         */
        bool OpenTransitionRule(Core::Int32 TransitionId);

        /**
         * @brief 중첩 그래프 탐색에서 직전 보기로 돌아갑니다.
         * @return 돌아갈 상위 보기가 있으면 true입니다.
         */
        bool NavigateBack();

        /**
         * @brief 탐색 경로의 지정 깊이까지 돌아가 해당 보기를 활성화합니다.
         * @param Depth 탐색할 상위 보기의 깊이 또는 노드 배치 깊이입니다.
         * @return 깊이가 유효하여 보기를 전환했으면 true입니다.
         */
        bool NavigateTo(std::size_t Depth);

    private:

        /**
         * @brief 열린 그래프와 중첩 탐색 상태·편집기 컨텍스트를 정리합니다.
         */
        void Close();

        /**
         * @brief 편집 사본을 파일로 저장한 뒤 원본에 적용합니다.
         * @return 저장했으면 true입니다.
         */
        bool Save();

        /**
         * @brief 그래프 변수 목록을 그립니다.
         * @param SaveRequested 이번 프레임에 저장 단축키가 눌려 활성 이름 입력도 확정할지 여부입니다.
         */
        void DrawVariables(bool SaveRequested);

        /**
         * @brief 변수 정의 편집 UI와 Getter 드래그 소스·삭제 버튼을 표시합니다.
         * @param Index 대상 배열의 항목 인덱스입니다.
         * @param SaveRequested 현재 프레임에 저장 단축키를 요청했는지 여부입니다.
         * @return 이 변수의 삭제 버튼을 눌렀으면 true입니다.
         */
        bool DrawVariable(std::size_t Index, bool SaveRequested);

        /**
         * @brief 노드 에디터 캔버스를 그리고 생성·연결·삭제를 처리합니다.
         */
        void DrawGraph();

        /**
         * @brief 포즈 그래프의 변수 Getter 노드를 표시합니다.
         */
        void DrawValueGetters();

        /**
         * @brief 변수 Getter에서 노드 파라미터로 이어지는 값 연결을 표시합니다.
         */
        void DrawValueLinks();

        /**
         * @brief 이름에 대응하는 그래프 변수 인덱스를 찾습니다.
         * @param Name 찾을 그래프 변수 이름입니다.
         * @return 변수 인덱스이며 없으면 -1입니다.
         */
        Core::Int32 FindVariableIndex(const Core::FString& Name) const;

        /**
         * @brief 선택한 노드의 이름과 파라미터를 그립니다.
         */
        void DrawSelection();

        /**
         * @brief 중첩 그래프의 탐색 경로와 상위 보기 이동 UI를 표시합니다.
         */
        void DrawNavigation();

        /**
         * @brief 상태 노드와 전이 링크를 그리고 생성·연결·선택 입력을 처리합니다.
         */
        void DrawStateMachine();

        /**
         * @brief Bool 전이 조건 노드와 값 연결을 그리고 편집 입력을 처리합니다.
         */
        void DrawTransitionRule();

        /**
         * @brief 선택한 상태 또는 전이의 설정 편집 UI를 표시합니다.
         */
        void DrawMachineSelection();

        /**
         * @brief 선택한 전이 조건 노드의 상수·변수·플레이어 설정을 표시합니다.
         */
        void DrawRuleSelection();

        /**
         * @brief 중첩 보기의 종류와 부모 관계에 맞는 탐색 표시 이름을 만듭니다.
         * @param View 표시할 그래프 보기 또는 장면 뷰입니다.
         * @param Parent 표시 이름 계산에 사용할 상위 보기이며 없으면 nullptr입니다.
         * @return 탐색 경로에 사용할 문자열입니다.
         */
        Core::FString GetViewLabel(const FGraphView& View, const FGraphView* Parent) const;

        /**
         * @brief 현재 보기를 탐색 경로에 보관하고 지정한 중첩 그래프로 이동합니다.
         * @param Graph 열거나 평가할 그래프입니다.
         * @param Mode 설정할 동작 또는 표시 모드입니다.
         * @param Machine 중첩 보기의 상태 머신 노드 ID입니다.
         * @param Transition 중첩 보기의 전이 ID입니다.
         * @param Label UI 항목의 표시 이름입니다.
         */
        void Navigate(Asset::UAnimGraph* Graph, EGraphView Mode, Core::Int32 Machine, Core::Int32 Transition, const Core::FString& Label);

        /**
         * @brief 지정한 그래프 보기와 노드 편집기 컨텍스트를 활성화합니다.
         * @param View 표시할 그래프 보기 또는 장면 뷰입니다.
         */
        void ActivateView(const FGraphView& View);

        /**
         * @brief 그래프별로 소유한 노드 편집기 컨텍스트를 해제합니다.
         */
        void ClearGraphContexts();

    private:

        /**
         * @brief 프로젝트 에셋 관리자입니다.
         */
        Asset::FAssetManager* mAssets{};

        /**
         * @brief 에셋 선택기에 쓸 목록입니다.
         */
        Editor::FEditorAssetCatalog* mCatalog{};

        /**
         * @brief 에셋 선택기에 쓸 썸네일 캐시입니다.
         */
        Editor::FAssetThumbnailCache* mThumbnails{};

        /**
         * @brief 파라미터용 에셋 선택기입니다.
         */
        Editor::FAssetPicker mPicker{};

        /**
         * @brief 노드 에디터 컨텍스트입니다.
         */
        ax::NodeEditor::EditorContext* mEditor{};

        /**
         * @brief 편집 사본입니다.
         */
        std::unique_ptr<Asset::UAnimGraph> mRootEditing{};

        /**
         * @brief 현재 중첩 보기에서 편집하는 비소유 그래프 포인터입니다.
         */
        Asset::UAnimGraph* mEditing{};

        /**
         * @brief 현재 활성 그래프 보기와 선택 상태입니다.
         */
        FGraphView mView{};

        /**
         * @brief 중첩 그래프 탐색을 복원하는 상위 보기 목록입니다.
         */
        Core::TArray<FGraphView> mNavigation{};

        /**
         * @brief 그래프 경로별로 소유하는 노드 편집기 컨텍스트입니다.
         */
        Core::TMap<Core::FString, ax::NodeEditor::EditorContext*> mGraphContexts{};

        /**
         * @brief 컨텍스트 메뉴를 연 노드 편집기 키입니다.
         */
        std::uintptr_t mContextNode{};

        /**
         * @brief Content 기준 경로입니다.
         */
        std::filesystem::path mPath{};

        /**
         * @brief 선택한 노드 식별자이며 없으면 -2입니다.
         */
        Core::Int32 mSelectedNode{-2};

        /**
         * @brief 다음 그리기에서 저장된 위치를 노드 에디터에 적용할지 여부입니다.
         */
        bool mApplyPositions{};

        /**
         * @brief 위치를 적용한 다음 프레임에 화면을 그래프에 맞출지 여부입니다.
         */
        bool mFitRequested{};

        /**
         * @brief 저장하지 않은 변경이 있는지 여부입니다.
         */
        bool mDirty{};

        /**
         * @brief 창이 열려 있는지 여부입니다.
         */
        bool mOpen{};

        /**
         * @brief 노드 추가 메뉴를 연 캔버스 위치입니다. 새 노드를 이 위치에 놓습니다.
         */
        float mCreateX{};

        /**
         * @brief 새 노드를 배치할 그래프 Y 좌표입니다.
         */
        float mCreateY{};

        /**
         * @brief 새 변수 이름 입력 버퍼입니다.
         */
        std::array<char, 64> mNewVariable{};

        /**
         * @brief 마지막 저장·편집 오류입니다.
         */
        Core::FString mError{};
    };

}
