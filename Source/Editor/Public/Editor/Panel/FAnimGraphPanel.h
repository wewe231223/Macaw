/**
 * @file FAnimGraphPanel.h
 * @brief 선택한 스켈레탈 메시 컴포넌트의 애니메이션 노드 그래프를 노드 에디터로 보여 주는 패널입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"
#include "Editor/Panel/FEditorPanel.h"

#include <unordered_set>

namespace ax::NodeEditor {
    struct EditorContext;
}

namespace Engine {
    class UAnimInstance;
    class USkeletalMeshComponent;
    struct FAnimNode_Base;
}

namespace Editor {

    /**
     * @brief 애니메이션 인스턴스의 노드 그래프를 언리얼 AnimGraph처럼 오른쪽의 Output Pose로 모이는 노드와 링크로 그립니다.
     * @details 그래프는 인스턴스의 루트 노드에서 포즈 입력을 따라가 찾습니다. 플레이 중에는 노드별 블렌드 가중치로 활성 노드와 링크를 강조하고,
     * 재생 시간·샘플 가중치 같은 값을 실시간으로 보여 주며, 바인딩되지 않은 입력 값은 노드 안에서 바로 바꿀 수 있습니다.
     * 편집 모드에서는 그래프 구조만 보여 주고, 인스턴스 설정은 Details 패널에서 편집합니다.
     *
     * 노드는 플레이마다 새로 만들어지므로 주소 대신 안정 식별자(그래프 에셋의 노드 식별자, 또는 인스턴스 안의 멤버 위치)로 구분합니다.
     * 처음 보는 노드만 배치하고(그래프 에셋이면 에디터에서 놓은 위치, 아니면 자동 배치), 이후에는 사용자가 옮긴 위치를 에디터를 끌 때까지 유지합니다.
     */
    class FAnimGraphPanel final : public Editor::FEditorPanel {
    public:
        /**
         * @brief 패널과 노드 에디터 컨텍스트를 만듭니다.
         * @param[in,out] Context 선택을 조회할 에디터 컨텍스트입니다.
         */
        explicit FAnimGraphPanel(Editor::FEditorContext* Context);

        /**
         * @brief 노드 에디터 컨텍스트를 해제합니다.
         */
        ~FAnimGraphPanel() override;

    public:
        /**
         * @brief 다음 그리기에서 노드를 다시 배치하고 그래프 전체가 보이게 합니다.
         */
        void RequestLayout();

    private:
        void DrawContent(float DeltaTime) override;

        /**
         * @brief 선택한 객체에서 표시할 스켈레탈 메시 컴포넌트를 찾습니다.
         * @return 선택한 컴포넌트이거나 선택한 액터의 첫 스켈레탈 메시 컴포넌트이며, 없으면 nullptr입니다.
         */
        Engine::USkeletalMeshComponent* FindTarget() const;

        /**
         * @brief 인스턴스 클래스의 리플렉션 변수와 소유 컴포넌트 상태를 그립니다.
         * @param[in,out] Instance 표시할 인스턴스입니다.
         * @param[in] Playing 플레이 중이면 true이며, 이때만 변수 값을 바로 바꿀 수 있습니다.
         */
        void DrawVariables(Engine::UAnimInstance& Instance, bool Playing);

        /**
         * @brief 노드 그래프를 그립니다.
         * @param[in,out] Instance 표시할 인스턴스입니다.
         * @param[in] Playing 플레이 중이면 true입니다.
         */
        void DrawGraph(Engine::UAnimInstance& Instance, bool Playing);

        /**
         * @brief 아직 배치하지 않은 노드에 위치를 정합니다.
         * @param[in] Instance 노드 위치를 제공할 수 있는 인스턴스입니다.
         * @param[in] Root 그래프의 루트 노드입니다.
         * @param[in] All 이미 배치한 노드도 다시 배치하려면 true입니다.
         * @return 새로 배치한 노드가 있으면 true입니다.
         */
        bool PlaceNodes(const Engine::UAnimInstance& Instance, Engine::FAnimNode_Base* Root, bool All);

    private:
        /**
         * @brief 노드 에디터 컨텍스트입니다. 패널이 소유합니다.
         */
        ax::NodeEditor::EditorContext* mEditor{};

        /**
         * @brief 이미 위치를 정한 노드의 안정 식별자입니다.
         * @details 플레이할 때마다 노드 객체는 새로 만들어지지만 식별자는 같으므로, 여기 있는 노드는 다시 배치하지 않고 사용자가 옮긴 위치를 유지합니다.
         */
        std::unordered_set<Core::Uint64> mPlacedNodes{};

        /**
         * @brief 다음 그리기에서 모든 노드를 다시 배치할지 여부입니다.
         */
        bool mLayoutRequested{};

        /**
         * @brief 배치 다음 프레임에 그래프 전체가 보이도록 화면을 맞출지 여부입니다. 노드 크기는 그린 뒤에 정해지므로 한 프레임 늦춥니다.
         */
        bool mFitRequested{};

        /**
         * @brief 변수 영역의 너비이며 픽셀 단위입니다.
         */
        float mVariablesWidth{260.0f};
    };

}
