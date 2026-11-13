/**
 * @file UAnimGraphInstance.h
 * @brief 애니메이션 그래프 에셋으로 노드를 만들어 실행하는 애니메이션 인스턴스입니다.
 * @ingroup Engine
 */

#pragma once

#include "Asset/Animation/UAnimGraph.h"
#include "Engine/Animation/UAnimInstance.h"

#include <memory>

namespace Engine {
    struct FAnimNode_AssetPlayerBase;
    struct FAnimValueNode_Base;
    struct FAnimGraphNodeBuildContext;

    /**
     * @brief 그래프 에셋의 변수 하나가 실행 중에 갖는 값입니다.
     */
    struct FAnimGraphRuntimeVariable {
    public:

        /**
         * @brief 변수 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 현재 값입니다. 변수 Getter가 이 값을 읽습니다.
         */
        float mValue{};

        /**
         * @brief 값의 출처입니다.
         */
        Asset::EAnimGraphVariableSource mSource{Asset::EAnimGraphVariableSource::Manual};

        /**
         * @brief 실행 그래프 변수의 Float 또는 Bool 형식입니다.
         */
        Asset::EAnimGraphVariableType mType{};

        /**
         * @brief Bool 상수 또는 런타임 변수 값입니다.
         */
        bool mBoolValue{};
    };

}

#include "UAnimGraphInstance.generated.h"

namespace Engine {

    /**
     * @brief 에디터에서 만든 UAnimGraph 에셋을 실행합니다.
     * @details 언리얼 애니메이션 블루프린트의 컴파일에 해당하는 일을 초기화 때 합니다. 에셋의 노드마다 FAnimGraphNodeRegistry로 실행 노드를 만들고,
     * 링크대로 포즈 입력을 연결한 뒤 Output에 연결된 노드를 루트로 지정합니다. 그래프 변수는 인스턴스가 값을 가지며, 묶은 노드 파라미터는 매 갱신 그 값을 읽습니다.
     *
     * PIE에서는 에셋을 저장한 다음 애니메이션 갱신에서 실행 노드를 다시 만듭니다.
     */
    UCLASS()
    class UAnimGraphInstance final : public Engine::UAnimInstance {
        GENERATED_BODY()

    public:

        /**
         * @brief UAnimGraphInstance의 초기 상태와 의존성을 구성합니다.
         */
        UAnimGraphInstance();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~UAnimGraphInstance() override;

    public:

        /**
         * @brief 실행할 애니메이션 그래프의 에셋 경로를 반환합니다.
         * @return 인스턴스가 보관하는 경로의 읽기 전용 참조입니다.
         */
        const Core::FString& GetAnimGraph() const;

        /**
         * @brief 실행할 애니메이션 그래프 경로를 지정하고 그래프를 재구성합니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @return 경로 설정을 허용하면 true입니다.
         */
        bool SetAnimGraph(const Core::FString& Path);

        /**
         * @brief 그래프 변수 값을 바꿉니다. OwnerSpeed 변수는 다음 갱신에서 덮어씁니다.
         * @param[in] Name 변수 이름입니다.
         * @param[in] Value 새 값입니다.
         * @return 변수가 있으면 true입니다.
         */
        bool SetFloatVariable(const Core::FString& Name, float Value);

        /**
         * @brief 그래프 변수 값을 반환합니다.
         * @param[in] Name 변수 이름입니다.
         * @return 변수가 없으면 0입니다.
         */
        float GetFloatVariable(const Core::FString& Name) const override;

        /**
         * @brief 외부에서 설정할 수 있는 Bool 그래프 변수의 값을 변경합니다.
         * @param Name 조회하거나 설정할 그래프 변수 이름입니다.
         * @param Value 표시·비교하거나 적용할 값입니다.
         * @return 이름·형식·공급 방식이 설정을 허용하면 true입니다.
         */
        bool SetBoolVariable(const Core::FString& Name, bool Value);

        /**
         * @brief 이름에 대응하는 Bool 그래프 변수 값을 조회합니다.
         * @param Name 조회하거나 설정할 그래프 변수 이름입니다.
         * @return 현재 변수 값이며 유효한 변수가 없으면 false입니다.
         */
        bool GetBoolVariable(const Core::FString& Name) const override;

        /**
         * @brief 지정한 이름과 Float·Bool 형식의 그래프 변수가 있는지 확인합니다.
         * @param Name 조회하거나 설정할 그래프 변수 이름입니다.
         * @param IsBool Bool 변수를 조회하면 true, Float 변수이면 false입니다.
         * @return 이름과 형식이 모두 일치하면 true입니다.
         */
        bool HasGraphVariable(const Core::FString& Name, bool IsBool) const override;

        /**
         * @brief 그래프 변수의 외부 값 공급 방식을 조회합니다.
         * @param Name 조회하거나 표시할 이름입니다.
         * @return 변수의 값 공급 종류이며 해당 변수가 없으면 Manual입니다.
         */
        Asset::EAnimGraphVariableSource GetVariableSource(const Core::FString& Name) const;

        /**
         * @brief 실행 그래프에서 이름에 대응하는 에셋 플레이어를 찾습니다.
         * @param Name 실행 그래프에서 찾을 에셋 플레이어 이름입니다.
         * @return 인스턴스 소유 플레이어의 비소유 포인터이며 없으면 nullptr입니다.
         */
        Engine::FAnimNode_AssetPlayerBase* FindAssetPlayer(const Core::FString& Name) const;

        void GetDebugVariables(Core::TArray<Engine::FAnimNodeValue>& OutValues) override;

        bool GetNodeLayout(const Engine::FAnimNode_Base* Node, float& X, float& Y) const override;

        /**
         * @brief 실행 포즈 노드에 대응하는 원본 에셋 노드를 조회합니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @return 원본 그래프 소유의 노드이며 대응 항목이 없으면 nullptr입니다.
         */
        const Asset::FAnimGraphNode* GetSourceNode(const Engine::FAnimNode_Base* Node) const;

        /**
         * @brief 실행 값 노드에 대응하는 원본 Getter를 조회합니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @return 원본 그래프 소유의 Getter이며 대응 항목이 없으면 nullptr입니다.
         */
        const Asset::FAnimGraphVariableGetter* GetSourceGetter(const Engine::FAnimValueNode_Base* Node) const;

        /**
         * @brief 중첩 그래프에서도 구분 가능한 실행 포즈 노드 키를 얻습니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @return 디버그·편집기 연결에 사용할 키이며 노드가 없으면 0입니다.
         */
        Core::Uint64 GetNodeKey(const Engine::FAnimNode_Base* Node) const;

        /**
         * @brief 실행 값 노드의 디버그 표시 키를 얻습니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @return 값 노드를 식별할 키이며 대응 항목이 없으면 0입니다.
         */
        Core::Uint64 GetValueNodeKey(const Engine::FAnimValueNode_Base* Node) const;

    private:

        /**
         * @brief 원본 포즈·값 노드와 중첩 상태 머신을 실행 노드로 구성합니다.
         * @param Source 복사하거나 읽을 원본 데이터입니다.
         * @param BaseContext 에셋·스켈레톤과 변수 조회를 제공하는 노드 구성 컨텍스트입니다.
         * @param Depth 탐색할 상위 보기의 깊이 또는 노드 배치 깊이입니다.
         * @return 인스턴스가 소유하는 루트 포즈 노드이며 실패하면 nullptr입니다.
         */
        Engine::FAnimNode_Base* BuildGraph(const Asset::UAnimGraph& Source, const Engine::FAnimGraphNodeBuildContext& BaseContext, Core::Uint32 Depth);

        void NativeInitializeAnimation() override;

        void NativeUpdateAnimation(float DeltaSeconds) override;

    private:

        /**
         * @brief 현재 실행 그래프의 원본 에셋에 대한 비소유 포인터입니다.
         */
        const Asset::UAnimGraph* mSourceGraph{};

        /**
         * @brief 실행 그래프를 구성할 때 확인한 원본 리비전입니다.
         */
        Core::FRevision mGraphRevision{};

        /**
         * @brief 실행할 그래프 에셋의 Content 기준 경로입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "AnimGraph", Getter = GetAnimGraph, Setter = SetAnimGraph)
        Core::FString mAnimGraph{};

        /**
         * @brief 그래프에서 만든 노드입니다.
         */
        Core::TArray<std::unique_ptr<Engine::FAnimNode_Base>> mNodes{};

        /**
         * @brief 실행 시 평가하는 값 노드의 소유 목록입니다.
         */
        Core::TArray<std::unique_ptr<Engine::FAnimValueNode_Base>> mValueNodes{};

        /**
         * @brief 실행 포즈 노드에 대응하는 원본 노드의 비소유 포인터입니다.
         */
        Core::TArray<const Asset::FAnimGraphNode*> mNodeSources{};

        /**
         * @brief 실행 포즈 노드가 속한 원본 그래프의 비소유 포인터입니다.
         */
        Core::TArray<const Asset::UAnimGraph*> mNodeGraphs{};

        /**
         * @brief 실행 값 노드에 대응하는 원본 Getter의 비소유 포인터입니다.
         */
        Core::TArray<const Asset::FAnimGraphVariableGetter*> mGetterSources{};

        /**
         * @brief 그래프 변수입니다.
         */
        Core::TArray<Engine::FAnimGraphRuntimeVariable> mVariables{};
    };

}
