/**
 * @file FAnimNode_StateMachine.h
 * @brief 상태별 포즈를 갱신하고 조건에 따른 전이와 루트 모션 블렌딩을 수행합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Asset/Animation/FAnimStateMachineData.h"
#include "Engine/Animation/FAnimNode_Base.h"

namespace Engine {

    /**
     * @brief 상태별 포즈 연결과 재진입 정책, 경과 시간을 보관하는 실행 데이터입니다.
     * @ingroup Engine
     */
    struct FAnimStateRuntime {
    public:

        /**
         * @brief 소속 데이터 안에서 항목을 식별하는 ID입니다.
         */
        Core::Int32 mId{};

        /**
         * @brief 플레이어 조회와 디버그 표시에 사용하는 상태 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 이 상태의 루트 포즈 노드에 대한 비소유 연결입니다.
         */
        Engine::FPoseLink mPose{};

        /**
         * @brief 상태에 다시 진입할 때 내부 포즈 그래프를 초기화할지 여부입니다.
         */
        bool mResetOnEntry{true};

        /**
         * @brief 현재 상태에서 경과한 시간(초)입니다.
         */
        float mElapsed{};
    };

    /**
     * @brief 상태별 포즈를 갱신하고 조건에 따른 전이와 루트 모션 블렌딩을 수행합니다.
     * @ingroup Engine
     */
    class FAnimNode_StateMachine final : public Engine::FAnimNode_Base {
    public:

        /**
         * @brief 에셋의 전이 목록과 진입 상태를 실행 상태 머신에 반영합니다.
         * @param Data 실행 상태로 구성할 에셋 데이터입니다.
         */
        void Configure(const Asset::FAnimStateMachineData& Data);

        /**
         * @brief 상태의 포즈 루트와 재진입 설정을 실행 배열에 등록합니다.
         * @param Data 실행 상태로 구성할 에셋 데이터입니다.
         * @param Root 해당 상태 또는 그래프의 비소유 루트 포즈 노드입니다.
         */
        void AddState(const Asset::FAnimStateData& Data, Engine::FAnimNode_Base* Root);

        void Initialize(Engine::UAnimInstance& AnimInstance) override;
        void Update(const Engine::FAnimationUpdateContext& Context) override;
        void Evaluate(Engine::FPoseContext& Output) override;

        const Core::FString& GetNodeType() const override;
        void GetPoseInputs(Core::TArray<Engine::FAnimNodePoseInput>& OutInputs) override;
        void GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues) override;

        /**
         * @brief 현재 활성 상태의 에셋 ID를 반환합니다.
         * @return 상태 ID이며 활성 상태가 없으면 -1입니다.
         */
        Core::Int32 GetCurrentState() const;

        /**
         * @brief 진행 중인 전이의 대상 상태 ID를 반환합니다.
         * @return 대상 상태 ID이며 전이가 없으면 -1입니다.
         */
        Core::Int32 GetTargetState() const;

        /**
         * @brief 진행 중인 전이의 정규화된 블렌드 진행도를 반환합니다.
         * @return 전이의 0~1 가중치입니다.
         */
        float GetBlendAlpha() const;

        /**
         * @brief 최근 갱신에서 계산한 루트 모션 증분을 반환합니다.
         * @return 노드 또는 인스턴스의 이동·회전 증분이며 없으면 항등 변환입니다.
         */
        Core::FTransform GetRootMotionDelta() const override;

    private:

        /**
         * @brief 에셋 상태 ID를 실행 상태 배열의 인덱스로 변환합니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 상태 인덱스이며 없으면 -1입니다.
         */
        Core::Int32 FindStateIndex(Core::Int32 Id) const;

    private:

        /**
         * @brief 상태 목록입니다.
         */
        Core::TArray<Engine::FAnimStateRuntime> mStates{};

        /**
         * @brief 상태 사이의 전이 목록입니다.
         */
        Core::TArray<Asset::FAnimTransitionData> mTransitions{};

        /**
         * @brief 전이 조건의 디버그 표시에 사용할 평가 값입니다.
         */
        Core::TArray<Engine::FAnimNodeValue> mRuleDebugValues{};

        /**
         * @brief 초기 진입 상태의 에셋 ID입니다.
         */
        Core::Int32 mEntry{-1};

        /**
         * @brief 현재 상태의 실행 배열 인덱스이며 미지정 시 -1입니다.
         */
        Core::Int32 mCurrent{-1};

        /**
         * @brief 전이 대상 상태의 실행 배열 인덱스이며 전이하지 않을 때 -1입니다.
         */
        Core::Int32 mTarget{-1};

        /**
         * @brief 진행 중인 전이의 경과 시간(초)입니다.
         */
        float mBlendElapsed{};

        /**
         * @brief 진행 중인 전이의 전체 블렌드 시간(초)입니다.
         */
        float mBlendDuration{};
    };

    /**
     * @brief 현재 상태의 재생 정보와 그래프 변수를 사용하여 전이 조건을 평가합니다.
     * @ingroup Engine
     */
    class FAnimTransitionEvaluator {
    public:

        /**
         * @brief 그래프의 Bool 결과 노드를 현재 상태 컨텍스트로 평가합니다.
         * @param Graph 열거나 평가할 그래프입니다.
         * @param Context 상태 시간과 변수·플레이어를 조회할 애니메이션 갱신 컨텍스트입니다.
         * @param StateRoot 현재 상태의 비소유 루트 포즈 노드입니다.
         * @param StateTime 현재 상태의 경과 시간(초)입니다.
         * @return 유효한 Bool 결과가 true일 때만 true입니다.
         */
        static bool Evaluate(const Asset::FAnimTransitionGraph& Graph, const Engine::FAnimationUpdateContext& Context, Engine::FAnimNode_Base* StateRoot, float StateTime);
    };

}
