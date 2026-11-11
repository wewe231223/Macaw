#pragma once

#include "Asset/Animation/FAnimStateMachineData.h"
#include "Engine/Animation/FAnimNode_Base.h"

namespace Engine {

    struct FAnimStateRuntime {
    public:
        Core::Int32 mId{};
        Core::FString mName{};
        Engine::FPoseLink mPose{};
        bool mResetOnEntry{true};
        float mElapsed{};
    };

    class FAnimNode_StateMachine final : public Engine::FAnimNode_Base {
    public:
        void Configure(const Asset::FAnimStateMachineData& Data);
        void AddState(const Asset::FAnimStateData& Data, Engine::FAnimNode_Base* Root);

        void Initialize(Engine::UAnimInstance& AnimInstance) override;
        void Update(const Engine::FAnimationUpdateContext& Context) override;
        void Evaluate(Engine::FPoseContext& Output) override;

        const Core::FString& GetNodeType() const override;
        void GetPoseInputs(Core::TArray<Engine::FAnimNodePoseInput>& OutInputs) override;
        void GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues) override;

        Core::Int32 GetCurrentState() const;
        Core::Int32 GetTargetState() const;
        float GetBlendAlpha() const;

    private:
        Core::Int32 FindStateIndex(Core::Int32 Id) const;

    private:
        Core::TArray<Engine::FAnimStateRuntime> mStates{};
        Core::TArray<Asset::FAnimTransitionData> mTransitions{};
        Core::TArray<Engine::FAnimNodeValue> mRuleDebugValues{};

        Core::Int32 mEntry{-1};
        Core::Int32 mCurrent{-1};
        Core::Int32 mTarget{-1};

        float mBlendElapsed{};
        float mBlendDuration{};
    };

    class FAnimTransitionEvaluator {
    public:
        static bool Evaluate(const Asset::FAnimTransitionGraph& Graph, const Engine::FAnimationUpdateContext& Context, Engine::FAnimNode_Base* StateRoot, float StateTime);
    };

}
