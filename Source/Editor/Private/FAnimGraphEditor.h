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
        enum class EGraphView {
            Pose,
            Machine,
            Rule
        };

        struct FGraphView {
            Asset::UAnimGraph* mGraph{};
            EGraphView mMode{};
            Core::Int32 mMachine{-1};
            Core::Int32 mTransition{-1};
            Core::Int32 mSelection{-2};
            Core::FString mLabel{"AnimGraph"};
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

        FAnimGraphEditor(const Editor::FAnimGraphEditor&) = delete;

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

        bool IsFocused() const;
        bool OpenStateMachine(Core::Int32 NodeId);
        bool OpenState(Core::Int32 StateId);
        bool OpenTransitionRule(Core::Int32 TransitionId);
        bool NavigateBack();
        bool NavigateTo(std::size_t Depth);

    private:
        void Close();

        /**
         * @brief 편집 사본을 파일로 저장한 뒤 원본에 적용합니다.
         * @return 저장했으면 true입니다.
         */
        bool Save();

        /**
         * @brief 그래프 변수 목록을 그립니다.
         */
        void DrawVariables(bool SaveRequested);
        bool DrawVariable(std::size_t Index, bool SaveRequested);

        /**
         * @brief 노드 에디터 캔버스를 그리고 생성·연결·삭제를 처리합니다.
         */
        void DrawGraph();
        void DrawValueGetters();
        void DrawValueLinks();
        Core::Int32 FindVariableIndex(const Core::FString& Name) const;

        /**
         * @brief 선택한 노드의 이름과 파라미터를 그립니다.
         */
        void DrawSelection();
        void DrawNavigation();
        void DrawStateMachine();
        void DrawTransitionRule();
        void DrawMachineSelection();
        void DrawRuleSelection();
        Core::FString GetViewLabel(const FGraphView& View, const FGraphView* Parent) const;
        void Navigate(Asset::UAnimGraph* Graph, EGraphView Mode, Core::Int32 Machine, Core::Int32 Transition, const Core::FString& Label);
        void ActivateView(const FGraphView& View);
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
        Asset::UAnimGraph* mEditing{};
        FGraphView mView{};
        Core::TArray<FGraphView> mNavigation{};
        Core::TMap<Core::FString, ax::NodeEditor::EditorContext*> mGraphContexts{};

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
