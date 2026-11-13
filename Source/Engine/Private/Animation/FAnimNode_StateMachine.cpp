/**
 * @file FAnimNode_StateMachine.cpp
 * @brief 상태별 포즈를 갱신하고 조건에 따른 전이와 루트 모션 블렌딩을 수행합니다.
 * @ingroup Engine
 */

#include "Engine/Animation/FAnimNode_StateMachine.h"
#include "Asset/Animation/UAnimGraph.h"
#include "Engine/Animation/FAnimNode_AssetPlayerBase.h"
#include "Engine/Animation/FAnimationRuntime.h"
#include "Engine/Animation/UAnimInstance.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <variant>

namespace Engine {

    /**
     * @brief 상태별 포즈를 갱신하고 조건에 따른 전이와 루트 모션 블렌딩을 수행하는 파일 내부 보조 구현입니다.
     */
    namespace {

        /**
         * @brief 전이 조건의 평가 실패·Float·Bool 결과를 구분하는 값 형식입니다.
         */
        using FRuleValue = std::variant<std::monostate, float, bool>;

        /**
         * @brief 상태 포즈 그래프에서 지정 이름의 에셋 플레이어를 찾습니다.
         * @param Root 해당 상태 또는 그래프의 비소유 루트 포즈 노드입니다.
         * @param Name 실행 그래프에서 찾을 에셋 플레이어 이름입니다.
         * @return 실행 그래프 소유의 플레이어이며 없으면 nullptr입니다.
         */
        Engine::FAnimNode_AssetPlayerBase* FindStatePlayer(Engine::FAnimNode_Base* Root, const Core::FString& Name) {
            Core::TArray<Engine::FAnimNode_Base*> Pending{Root};
            Core::TArray<Engine::FAnimNode_Base*> Visited{};
            Engine::FAnimNode_AssetPlayerBase* Found{};

            while (!Pending.empty()) {
                auto* Node{Pending.back()};
                Pending.pop_back();

                if (Node == nullptr || std::find(Visited.begin(), Visited.end(), Node) != Visited.end()) {
                    continue;
                }

                Visited.push_back(Node);

                if (auto* Player{dynamic_cast<Engine::FAnimNode_AssetPlayerBase*>(Node)}; Player != nullptr && (Name.empty() || Node->mName == Name)) {
                    if (Found != nullptr) {
                        return nullptr;
                    }

                    Found = Player;
                }

                Core::TArray<Engine::FAnimNodePoseInput> Inputs{};
                Node->GetPoseInputs(Inputs);

                for (const auto& Input : Inputs) {
                    Pending.push_back(Input.mLink->mLinkedNode);
                }
            }

            return Found;
        }

        /**
         * @brief 입력 노드를 재귀 평가하고 결과를 캐시하여 전이 조건 값을 계산합니다.
         * @param Graph 열거나 평가할 그래프입니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @param Context 시간과 인스턴스를 제공하는 애니메이션 갱신 컨텍스트입니다.
         * @param Root 해당 상태 또는 그래프의 비소유 루트 포즈 노드입니다.
         * @param StateTime 현재 상태의 경과 시간(초)입니다.
         * @param Cache 동일 종류 에셋의 경로별 캐시입니다.
         * @param Depth 탐색할 상위 보기의 깊이 또는 노드 배치 깊이입니다.
         * @return Float 또는 Bool 값이며 평가할 수 없으면 monostate입니다.
         */
        FRuleValue EvaluateRuleNode(const Asset::FAnimTransitionGraph& Graph, Core::Int32 Id, const Engine::FAnimationUpdateContext& Context, Engine::FAnimNode_Base* Root, float StateTime, Core::TMap<Core::Int32, FRuleValue>& Cache, Core::Uint32 Depth) {
            if (Depth >= Asset::UAnimGraph::MaxNodes) {
                return {};
            }

            if (const auto Found{Cache.find(Id)}; Found != Cache.end()) {
                return Found->second;
            }

            const auto* Node{Graph.FindNode(Id)};

            if (Node == nullptr) {
                return {};
            }

            FRuleValue Value{};
            Engine::UAnimInstance& Instance{Context.GetAnimInstance()};

            switch (Node->mType) {
                case Asset::EAnimRuleNodeType::Float: Value = Node->mValue; break;
                case Asset::EAnimRuleNodeType::Bool: Value = Node->mBoolValue; break;
                case Asset::EAnimRuleNodeType::GetFloat: {
                    if (Instance.HasGraphVariable(Node->mName, false)) {
                        Value = Instance.GetFloatVariable(Node->mName);
                    }

                    break;
                }
                case Asset::EAnimRuleNodeType::GetBool: {
                    if (Instance.HasGraphVariable(Node->mName, true)) {
                        Value = Instance.GetBoolVariable(Node->mName);
                    }

                    break;
                }
                case Asset::EAnimRuleNodeType::StateTime: Value = StateTime; break;
                case Asset::EAnimRuleNodeType::TimeRemaining:
                case Asset::EAnimRuleNodeType::Finished: {
                    const auto* Player{FindStatePlayer(Root, Node->mName)};

                    if (Player != nullptr && Player->GetCurrentAssetLength() > 0.0) {
                        if (Node->mType == Asset::EAnimRuleNodeType::Finished) {
                            Value = Player->IsFinished();
                        } else {
                            const double Remaining{Player->mPlayRate < 0.0f ? Player->GetAccumulatedTime() : Player->GetCurrentAssetLength() - Player->GetAccumulatedTime()};
                            Value = std::abs(Player->mPlayRate) > 0.000001f ? static_cast<float>(std::max(Remaining, 0.0) / std::abs(Player->mPlayRate)) : std::numeric_limits<float>::max();
                        }
                    }

                    break;
                }
                default: {
                    const FRuleValue A{EvaluateRuleNode(Graph, Node->mInputA, Context, Root, StateTime, Cache, Depth + 1)};
                    const FRuleValue B{Node->mType != Asset::EAnimRuleNodeType::Not ? EvaluateRuleNode(Graph, Node->mInputB, Context, Root, StateTime, Cache, Depth + 1) : FRuleValue{}};

                    if (Asset::FAnimTransitionGraph::IsBoolInput(Node->mType)) {
                        const auto* Left{std::get_if<bool>(&A)};
                        const auto* Right{std::get_if<bool>(&B)};

                        if (Left != nullptr && (Right != nullptr || Node->mType == Asset::EAnimRuleNodeType::Not)) {
                            if (Node->mType == Asset::EAnimRuleNodeType::Not) {
                                Value = !*Left;
                            } else {
                                Value = Node->mType == Asset::EAnimRuleNodeType::And ? *Left && *Right : *Left || *Right;
                            }
                        }
                    } else {
                        const auto* Left{std::get_if<float>(&A)};
                        const auto* Right{std::get_if<float>(&B)};

                        if (Left != nullptr && Right != nullptr && std::isfinite(*Left) && std::isfinite(*Right)) {
                            switch (Node->mType) {
                                case Asset::EAnimRuleNodeType::Greater: Value = *Left > *Right; break;
                                case Asset::EAnimRuleNodeType::GreaterEqual: Value = *Left >= *Right; break;
                                case Asset::EAnimRuleNodeType::Less: Value = *Left < *Right; break;
                                case Asset::EAnimRuleNodeType::LessEqual: Value = *Left <= *Right; break;
                                case Asset::EAnimRuleNodeType::Equal: Value = *Left == *Right; break;
                                case Asset::EAnimRuleNodeType::NotEqual: Value = *Left != *Right; break;
                                default: break;
                            }
                        }
                    }

                    break;
                }
            }

            Cache.emplace(Id, Value);

            return Value;
        }
    }

    bool FAnimTransitionEvaluator::Evaluate(const Asset::FAnimTransitionGraph& Graph, const Engine::FAnimationUpdateContext& Context, Engine::FAnimNode_Base* StateRoot, float StateTime) {
        Core::TMap<Core::Int32, FRuleValue> Cache{};
        const FRuleValue Value{EvaluateRuleNode(Graph, Graph.GetResult(), Context, StateRoot, StateTime, Cache, 0)};
        const auto* Result{std::get_if<bool>(&Value)};

        return Result != nullptr && *Result;
    }

    void FAnimNode_StateMachine::Configure(const Asset::FAnimStateMachineData& Data) {
        mEntry = Data.GetEntryState();
        mTransitions = Data.GetTransitions();
        std::stable_sort(mTransitions.begin(), mTransitions.end(), [](const auto& A, const auto& B) {
            return A.mPriority < B.mPriority;
        });
    }

    void FAnimNode_StateMachine::AddState(const Asset::FAnimStateData& Data, Engine::FAnimNode_Base* Root) {
        mStates.push_back({Data.mId, Data.mName, {Root}, Data.mResetOnEntry, 0.0f});
    }

    Core::Int32 FAnimNode_StateMachine::FindStateIndex(Core::Int32 Id) const {
        const auto Found{std::find_if(mStates.begin(), mStates.end(), [Id](const auto& State) {
            return State.mId == Id;
        })};

        return Found != mStates.end() ? static_cast<Core::Int32>(Found - mStates.begin()) : -1;
    }

    void FAnimNode_StateMachine::Initialize(Engine::UAnimInstance& AnimInstance) {
        Engine::FAnimNode_Base::Initialize(AnimInstance);
        mCurrent = FindStateIndex(mEntry);
        mTarget = -1;
        mBlendElapsed = 0.0f;
        mBlendDuration = 0.0f;
        mRuleDebugValues.clear();

        for (auto& State : mStates) {
            State.mElapsed = 0.0f;
        }
    }

    void FAnimNode_StateMachine::Update(const Engine::FAnimationUpdateContext& Context) {
        if (mCurrent < 0) {
            return;
        }

        const float Delta{std::isfinite(Context.GetDeltaTime()) ? std::max(Context.GetDeltaTime(), 0.0f) : 0.0f};
        auto& Current{mStates[static_cast<std::size_t>(mCurrent)]};
        Current.mPose.Update(Context);
        Current.mElapsed += Delta;

        if (mTarget >= 0) {
            auto& Target{mStates[static_cast<std::size_t>(mTarget)]};
            Target.mPose.Update(Context);
            Target.mElapsed += Delta;
            mBlendElapsed += Delta;

            if (mBlendElapsed >= mBlendDuration) {
                mCurrent = mTarget;
                mTarget = -1;
            }

            return;
        }

        mRuleDebugValues.clear();

        for (const auto& Transition : mTransitions) {
            const Core::Int32 TargetIndex{FindStateIndex(Transition.mToState)};

            if (Transition.mFromState != Current.mId || TargetIndex < 0) {
                continue;
            }

            const bool Allowed{Engine::FAnimTransitionEvaluator::Evaluate(Transition.mRule, Context, Current.mPose.mLinkedNode, Current.mElapsed)};
            mRuleDebugValues.push_back({std::format("Rule -> {} ({})", mStates[static_cast<std::size_t>(TargetIndex)].mName, Transition.mPriority), Core::FString{Allowed ? "true" : "false"}});

            if (!Allowed) {
                continue;
            }

            auto& Target{mStates[static_cast<std::size_t>(TargetIndex)]};

            if (Target.mResetOnEntry) {
                Target.mPose.Initialize(Context.GetAnimInstance());
            }

            Target.mElapsed = 0.0f;
            const Engine::FAnimationUpdateContext InitialContext{Context.GetAnimInstance(), 0.0f};
            Target.mPose.Update(InitialContext);
            mBlendElapsed = 0.0f;
            mBlendDuration = std::max(Transition.mDuration, 0.0f);

            if (mBlendDuration <= 0.0f || TargetIndex == mCurrent) {
                mCurrent = TargetIndex;
            } else {
                mTarget = TargetIndex;
            }

            break;
        }
    }

    void FAnimNode_StateMachine::Evaluate(Engine::FPoseContext& Output) {
        if (mCurrent < 0) {
            return;
        }

        auto& Current{mStates[static_cast<std::size_t>(mCurrent)]};

        if (mTarget < 0) {
            Current.mPose.Evaluate(Output);

            return;
        }

        Engine::FPoseContext Source{Output.GetAnimInstance()};
        Engine::FPoseContext Target{Output.GetAnimInstance()};
        const float Alpha{GetBlendAlpha()};
        Current.mPose.Evaluate(Source, 1.0f - Alpha);
        mStates[static_cast<std::size_t>(mTarget)].mPose.Evaluate(Target, Alpha);
        Engine::FAnimationRuntime::BlendTwoPosesTogether(Source.mPose, Target.mPose, 1.0f - Alpha, Output.mPose);
    }

    const Core::FString& FAnimNode_StateMachine::GetNodeType() const {
        static const Core::FString Type{"State Machine"};

        return Type;
    }

    void FAnimNode_StateMachine::GetPoseInputs(Core::TArray<Engine::FAnimNodePoseInput>& OutInputs) {
        for (auto& State : mStates) {
            OutInputs.push_back({State.mName, &State.mPose});
        }
    }

    void FAnimNode_StateMachine::GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues) {
        OutValues.push_back({"Current State", mCurrent >= 0 ? mStates[static_cast<std::size_t>(mCurrent)].mName : Core::FString{"None"}});
        OutValues.push_back({"Target State", mTarget >= 0 ? mStates[static_cast<std::size_t>(mTarget)].mName : Core::FString{"None"}});
        OutValues.push_back({"Transition", std::format("{:.0f}%", GetBlendAlpha() * 100.0f)});

        if (mCurrent >= 0) {
            OutValues.push_back({"State Time", std::format("{:.2f} s", mStates[static_cast<std::size_t>(mCurrent)].mElapsed)});
        }

        if (mTarget >= 0) {
            OutValues.push_back({"Rules", Core::FString{"Deferred while blending"}});
        } else {
            OutValues.insert(OutValues.end(), mRuleDebugValues.begin(), mRuleDebugValues.end());
        }
    }

    Core::Int32 FAnimNode_StateMachine::GetCurrentState() const {
        return mCurrent >= 0 ? mStates[static_cast<std::size_t>(mCurrent)].mId : -1;
    }

    Core::Int32 FAnimNode_StateMachine::GetTargetState() const {
        return mTarget >= 0 ? mStates[static_cast<std::size_t>(mTarget)].mId : -1;
    }

    float FAnimNode_StateMachine::GetBlendAlpha() const {
        return mTarget >= 0 && mBlendDuration > 0.0f ? std::clamp(mBlendElapsed / mBlendDuration, 0.0f, 1.0f) : 1.0f;
    }

    Core::FTransform FAnimNode_StateMachine::GetRootMotionDelta() const {
        if (mCurrent < 0) {
            return {};
        }

        const Engine::FAnimNode_Base* Current{mStates[static_cast<std::size_t>(mCurrent)].mPose.mLinkedNode};
        const Core::FTransform Source{Current != nullptr ? Current->GetRootMotionDelta() : Core::FTransform{}};

        if (mTarget < 0) {
            return Source;
        }

        const Engine::FAnimNode_Base* Target{mStates[static_cast<std::size_t>(mTarget)].mPose.mLinkedNode};

        return Engine::FAnimationRuntime::BlendTransforms(Source, Target != nullptr ? Target->GetRootMotionDelta() : Core::FTransform{}, 1.0f - GetBlendAlpha());
    }

}
