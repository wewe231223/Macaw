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
        Asset::EAnimGraphVariableType mType{};
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
        UAnimGraphInstance();

        ~UAnimGraphInstance() override;

    public:
        const Core::FString& GetAnimGraph() const;
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
        bool SetBoolVariable(const Core::FString& Name, bool Value);
        bool GetBoolVariable(const Core::FString& Name) const override;
        bool HasGraphVariable(const Core::FString& Name, bool IsBool) const override;
        Asset::EAnimGraphVariableSource GetVariableSource(const Core::FString& Name) const;

        Engine::FAnimNode_AssetPlayerBase* FindAssetPlayer(const Core::FString& Name) const;

        void GetDebugVariables(Core::TArray<Engine::FAnimNodeValue>& OutValues) override;

        bool GetNodeLayout(const Engine::FAnimNode_Base* Node, float& X, float& Y) const override;
        const Asset::FAnimGraphNode* GetSourceNode(const Engine::FAnimNode_Base* Node) const;
        const Asset::FAnimGraphVariableGetter* GetSourceGetter(const Engine::FAnimValueNode_Base* Node) const;
        Core::Uint64 GetNodeKey(const Engine::FAnimNode_Base* Node) const;
        Core::Uint64 GetValueNodeKey(const Engine::FAnimValueNode_Base* Node) const;

    private:
        Engine::FAnimNode_Base* BuildGraph(const Asset::UAnimGraph& Source, const Engine::FAnimGraphNodeBuildContext& BaseContext, Core::Uint32 Depth);

        void NativeInitializeAnimation() override;

        void NativeUpdateAnimation(float DeltaSeconds) override;

    private:
        const Asset::UAnimGraph* mSourceGraph{};
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
        Core::TArray<std::unique_ptr<Engine::FAnimValueNode_Base>> mValueNodes{};
        Core::TArray<const Asset::FAnimGraphNode*> mNodeSources{};
        Core::TArray<const Asset::UAnimGraph*> mNodeGraphs{};
        Core::TArray<const Asset::FAnimGraphVariableGetter*> mGetterSources{};

        /**
         * @brief 그래프 변수입니다.
         */
        Core::TArray<Engine::FAnimGraphRuntimeVariable> mVariables{};
    };

}
