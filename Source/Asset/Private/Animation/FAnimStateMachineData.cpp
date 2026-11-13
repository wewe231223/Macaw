/**
 * @file FAnimStateMachineData.cpp
 * @brief 상태별 포즈 그래프와 전이 조건을 보관하고 직렬화하는 상태 머신 에셋 데이터입니다.
 * @ingroup Asset
 */

#include "Asset/Animation/FAnimStateMachineData.h"
#include "Asset/Animation/UAnimGraph.h"
#include "Core/Archive.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace Asset {

    const Core::TArray<Asset::FAnimRuleNode>& FAnimTransitionGraph::GetNodes() const {
        return mNodes;
    }

    Asset::FAnimRuleNode* FAnimTransitionGraph::FindNode(Core::Int32 Id) {
        return const_cast<Asset::FAnimRuleNode*>(std::as_const(*this).FindNode(Id));
    }

    const Asset::FAnimRuleNode* FAnimTransitionGraph::FindNode(Core::Int32 Id) const {
        const auto Found{std::find_if(mNodes.begin(), mNodes.end(), [Id](const auto& Node) {
            return Node.mId == Id;
        })};

        return Found != mNodes.end() ? &*Found : nullptr;
    }

    Core::Int32 FAnimTransitionGraph::AddNode(Asset::EAnimRuleNodeType Type, float X, float Y) {
        if (Type >= Asset::EAnimRuleNodeType::Count || mNodes.size() >= Asset::UAnimGraph::MaxNodes || mNextId == std::numeric_limits<Core::Int32>::max() || !std::isfinite(X) || !std::isfinite(Y)) {
            return -1;
        }

        Asset::FAnimRuleNode Node{};
        Node.mId = mNextId++;
        Node.mType = Type;
        Node.mX = X;
        Node.mY = Y;
        mNodes.push_back(Node);

        return Node.mId;
    }

    bool FAnimTransitionGraph::RemoveNode(Core::Int32 Id) {
        if (std::erase_if(mNodes, [Id](const auto& Node) {
            return Node.mId == Id;
        }) == 0) {
            return false;
        }

        for (auto& Node : mNodes) {
            if (Node.mInputA == Id) {
                Node.mInputA = -1;
            }

            if (Node.mInputB == Id) {
                Node.mInputB = -1;
            }
        }

        if (mResult == Id) {
            mResult = -1;
        }

        return true;
    }

    bool FAnimTransitionGraph::Reaches(Core::Int32 From, Core::Int32 To) const {
        Core::TArray<Core::Int32> Pending{From};
        Core::TArray<Core::Int32> Visited{};

        while (!Pending.empty()) {
            const Core::Int32 Id{Pending.back()};
            Pending.pop_back();

            if (Id == To) {
                return true;
            }

            const auto* Node{FindNode(Id)};

            if (Node == nullptr || std::find(Visited.begin(), Visited.end(), Id) != Visited.end()) {
                continue;
            }

            Visited.push_back(Id);
            Pending.push_back(Node->mInputA);
            Pending.push_back(Node->mInputB);
        }

        return false;
    }

    bool FAnimTransitionGraph::Connect(Core::Int32 From, Core::Int32 To, Core::Int32 Pin) {
        const auto* Source{FindNode(From)};
        auto* Target{FindNode(To)};

        if (Source == nullptr || Target == nullptr || Pin < 0 || Pin >= GetInputCount(Target->mType) || IsBool(Source->mType) != IsBoolInput(Target->mType) || Reaches(From, To)) {
            return false;
        }

        (Pin == 0 ? Target->mInputA : Target->mInputB) = From;

        return true;
    }

    bool FAnimTransitionGraph::Disconnect(Core::Int32 To, Core::Int32 Pin) {
        auto* Node{FindNode(To)};

        if (Node == nullptr || Pin < 0 || Pin >= GetInputCount(Node->mType)) {
            return false;
        }

        Core::Int32& Input{Pin == 0 ? Node->mInputA : Node->mInputB};
        const bool Changed{Input >= 0};
        Input = -1;

        return Changed;
    }

    bool FAnimTransitionGraph::SetResult(Core::Int32 Id) {
        const auto* Node{FindNode(Id)};

        if (Id != -1 && (Node == nullptr || !IsBool(Node->mType))) {
            return false;
        }

        mResult = Id;

        return true;
    }

    Core::Int32 FAnimTransitionGraph::GetResult() const {
        return mResult;
    }

    bool FAnimTransitionGraph::IsBool(Asset::EAnimRuleNodeType Type) {
        return Type == Asset::EAnimRuleNodeType::Bool || Type == Asset::EAnimRuleNodeType::GetBool || (Type >= Asset::EAnimRuleNodeType::Greater && Type <= Asset::EAnimRuleNodeType::Not) || Type == Asset::EAnimRuleNodeType::Finished;
    }

    Core::Int32 FAnimTransitionGraph::GetInputCount(Asset::EAnimRuleNodeType Type) {
        return Type == Asset::EAnimRuleNodeType::Not ? 1 : (Type >= Asset::EAnimRuleNodeType::Greater && Type <= Asset::EAnimRuleNodeType::Or ? 2 : 0);
    }

    bool FAnimTransitionGraph::IsBoolInput(Asset::EAnimRuleNodeType Type) {
        return Type == Asset::EAnimRuleNodeType::And || Type == Asset::EAnimRuleNodeType::Or || Type == Asset::EAnimRuleNodeType::Not;
    }

    const char* FAnimTransitionGraph::GetTitle(Asset::EAnimRuleNodeType Type) {
        static constexpr const char* Titles[]{"Float", "Bool", "Get Float", "Get Bool", ">", ">=", "<", "<=", "==", "!=", "And", "Or", "Not", "State Time", "Time Remaining", "Finished"};

        return Type < Asset::EAnimRuleNodeType::Count ? Titles[static_cast<std::size_t>(Type)] : "Unknown";
    }

    bool FAnimTransitionGraph::Validate() const {
        if (mNodes.size() > Asset::UAnimGraph::MaxNodes || (mResult != -1 && (FindNode(mResult) == nullptr || !IsBool(FindNode(mResult)->mType)))) {
            return false;
        }

        for (std::size_t Index{}; Index < mNodes.size(); ++Index) {
            const auto& Node{mNodes[Index]};

            if (Node.mId < 0 || Node.mId == std::numeric_limits<Core::Int32>::max() || Node.mType >= Asset::EAnimRuleNodeType::Count || !std::isfinite(Node.mValue) || !std::isfinite(Node.mX) || !std::isfinite(Node.mY) || std::any_of(mNodes.begin(), mNodes.begin() + Index, [&Node](const auto& Other) {
                return Other.mId == Node.mId;
            })) {
                return false;
            }

            for (Core::Int32 Pin{}; Pin < 2; ++Pin) {
                const Core::Int32 Input{Pin == 0 ? Node.mInputA : Node.mInputB};

                if (Input == -1) {
                    continue;
                }

                const auto* Source{FindNode(Input)};

                if (Pin >= GetInputCount(Node.mType) || Source == nullptr || IsBool(Source->mType) != IsBoolInput(Node.mType) || Reaches(Input, Node.mId)) {
                    return false;
                }
            }
        }

        return true;
    }

    void FAnimTransitionGraph::RenameVariable(const Core::FString& OldName, const Core::FString& NewName) {
        for (auto& Node : mNodes) {
            if ((Node.mType == Asset::EAnimRuleNodeType::GetFloat || Node.mType == Asset::EAnimRuleNodeType::GetBool) && Node.mName == OldName) {
                Node.mName = NewName;
            }
        }
    }

    void FAnimTransitionGraph::Serialize(Core::FArchive& Archive) {
        Archive.Serialize("Result", mResult);
        Core::Uint32 Count{static_cast<Core::Uint32>(mNodes.size())};

        if (!Archive.BeginArray("Nodes", Count)) {
            return;
        }

        if (Count > Asset::UAnimGraph::MaxNodes) {
            Archive.SetError("Too many transition rule nodes");
        } else if (Archive.IsLoading()) {
            mNodes.assign(Count, Asset::FAnimRuleNode{});
            mNextId = 0;
        }

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            auto& Node{mNodes[Index]};
            Core::Uint32 Type{static_cast<Core::Uint32>(Node.mType)};
            Archive.Serialize("Id", Node.mId);
            Archive.Serialize("Type", Type);
            Archive.Serialize("A", Node.mInputA);
            Archive.Serialize("B", Node.mInputB);
            Archive.Serialize("Name", Node.mName);
            Archive.Serialize("Value", Node.mValue);
            Archive.Serialize("Bool", Node.mBoolValue);
            Archive.Serialize("X", Node.mX);
            Archive.Serialize("Y", Node.mY);
            Archive.EndObject();

            if (Type >= static_cast<Core::Uint32>(Asset::EAnimRuleNodeType::Count)) {
                Archive.SetError("Invalid transition rule node type");
            } else {
                Node.mType = static_cast<Asset::EAnimRuleNodeType>(Type);
            }

            if (Archive.IsLoading() && Node.mId >= 0 && Node.mId < std::numeric_limits<Core::Int32>::max()) {
                mNextId = std::max(mNextId, Node.mId + 1);
            }
        }

        Archive.EndArray();

        if (!Archive.HasError() && !Validate()) {
            Archive.SetError("Invalid transition rule graph");
        }
    }

    const Core::TArray<Asset::FAnimStateData>& FAnimStateMachineData::GetStates() const {
        return mStates;
    }

    const Core::TArray<Asset::FAnimTransitionData>& FAnimStateMachineData::GetTransitions() const {
        return mTransitions;
    }

    Asset::FAnimStateData* FAnimStateMachineData::FindState(Core::Int32 Id) {
        return const_cast<Asset::FAnimStateData*>(std::as_const(*this).FindState(Id));
    }

    const Asset::FAnimStateData* FAnimStateMachineData::FindState(Core::Int32 Id) const {
        const auto Found{std::find_if(mStates.begin(), mStates.end(), [Id](const auto& State) {
            return State.mId == Id;
        })};

        return Found != mStates.end() ? &*Found : nullptr;
    }

    Asset::FAnimTransitionData* FAnimStateMachineData::FindTransition(Core::Int32 Id) {
        return const_cast<Asset::FAnimTransitionData*>(std::as_const(*this).FindTransition(Id));
    }

    const Asset::FAnimTransitionData* FAnimStateMachineData::FindTransition(Core::Int32 Id) const {
        const auto Found{std::find_if(mTransitions.begin(), mTransitions.end(), [Id](const auto& Transition) {
            return Transition.mId == Id;
        })};

        return Found != mTransitions.end() ? &*Found : nullptr;
    }

    Core::Int32 FAnimStateMachineData::AddState(const Core::FString& Name, float X, float Y, const Asset::UAnimGraph& Owner) {
        if (Name.empty() || mStates.size() >= MaxStates || mNextStateId == std::numeric_limits<Core::Int32>::max() || !std::isfinite(X) || !std::isfinite(Y) || std::any_of(mStates.begin(), mStates.end(), [&Name](const auto& State) {
            return State.mName == Name;
        })) {
            return -1;
        }

        Asset::FAnimStateData State{};
        State.mId = mNextStateId++;
        State.mName = Name;
        State.mX = X;
        State.mY = Y;
        State.mGraph = std::make_shared<Asset::UAnimGraph>();

        for (const auto& Variable : Owner.GetVariables()) {
            State.mGraph->AddVariable(Variable);
        }

        mStates.push_back(State);

        if (mEntryState < 0) {
            mEntryState = State.mId;
        }

        return State.mId;
    }

    bool FAnimStateMachineData::RemoveState(Core::Int32 Id) {
        if (std::erase_if(mStates, [Id](const auto& State) {
            return State.mId == Id;
        }) == 0) {
            return false;
        }

        std::erase_if(mTransitions, [Id](const auto& Transition) {
            return Transition.mFromState == Id || Transition.mToState == Id;
        });

        if (mEntryState == Id) {
            mEntryState = mStates.empty() ? -1 : mStates.front().mId;
        }

        return true;
    }

    Core::Int32 FAnimStateMachineData::AddTransition(Core::Int32 From, Core::Int32 To) {
        if (FindState(From) == nullptr || FindState(To) == nullptr || mTransitions.size() >= Asset::UAnimGraph::MaxNodes || mNextTransitionId == std::numeric_limits<Core::Int32>::max()) {
            return -1;
        }

        Asset::FAnimTransitionData Transition{};
        Transition.mId = mNextTransitionId++;
        Transition.mFromState = From;
        Transition.mToState = To;
        mTransitions.push_back(Transition);

        return Transition.mId;
    }

    bool FAnimStateMachineData::RemoveTransition(Core::Int32 Id) {
        return std::erase_if(mTransitions, [Id](const auto& Transition) {
            return Transition.mId == Id;
        }) != 0;
    }

    bool FAnimStateMachineData::SetEntryState(Core::Int32 Id) {
        if (FindState(Id) == nullptr) {
            return false;
        }

        mEntryState = Id;

        return true;
    }

    Core::Int32 FAnimStateMachineData::GetEntryState() const {
        return mEntryState;
    }

    void FAnimStateMachineData::CopyFrom(const Asset::FAnimStateMachineData& Source) {
        if (this == &Source) {
            return;
        }

        mStates = Source.mStates;
        mTransitions = Source.mTransitions;
        mEntryState = Source.mEntryState;
        mNextStateId = Source.mNextStateId;
        mNextTransitionId = Source.mNextTransitionId;

        for (auto& State : mStates) {
            auto Graph{std::make_shared<Asset::UAnimGraph>()};

            if (State.mGraph != nullptr) {
                Graph->CopyFrom(*State.mGraph);
            }

            State.mGraph = std::move(Graph);
        }
    }

    void FAnimStateMachineData::Serialize(Core::FArchive& Archive) {
        Archive.Serialize("Entry", mEntryState);
        Core::Uint32 Count{static_cast<Core::Uint32>(mStates.size())};

        if (!Archive.BeginArray("States", Count)) {
            return;
        }

        if (Count > MaxStates) {
            Archive.SetError("Too many animation states");
        } else if (Archive.IsLoading()) {
            mStates.assign(Count, Asset::FAnimStateData{});
            mNextStateId = 0;
        }

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            auto& State{mStates[Index]};
            Archive.Serialize("Id", State.mId);
            Archive.Serialize("Name", State.mName);
            Archive.Serialize("X", State.mX);
            Archive.Serialize("Y", State.mY);
            Archive.Serialize("ResetOnEntry", State.mResetOnEntry);

            if (Archive.IsLoading()) {
                State.mGraph = std::make_shared<Asset::UAnimGraph>();
            }

            if (State.mGraph == nullptr) {
                Archive.SetError("Missing state pose graph");
            } else if (Archive.BeginObject("Graph")) {
                State.mGraph->Serialize(Archive);
                Archive.EndObject();
            }

            Archive.EndObject();

            if (State.mId < 0 || State.mId == std::numeric_limits<Core::Int32>::max() || State.mName.empty() || !std::isfinite(State.mX) || !std::isfinite(State.mY) || std::any_of(mStates.begin(), mStates.begin() + Index, [&State](const auto& Other) {
                return Other.mId == State.mId || Other.mName == State.mName;
            })) {
                Archive.SetError("Invalid or duplicate animation state");
            } else {
                mNextStateId = std::max(mNextStateId, State.mId + 1);
            }
        }

        Archive.EndArray();
        Count = static_cast<Core::Uint32>(mTransitions.size());

        if (Archive.HasError() || !Archive.BeginArray("Transitions", Count)) {
            return;
        }

        if (Count > Asset::UAnimGraph::MaxNodes) {
            Archive.SetError("Too many animation transitions");
        } else if (Archive.IsLoading()) {
            mTransitions.assign(Count, Asset::FAnimTransitionData{});
            mNextTransitionId = 0;
        }

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            auto& Transition{mTransitions[Index]};
            Archive.Serialize("Id", Transition.mId);
            Archive.Serialize("From", Transition.mFromState);
            Archive.Serialize("To", Transition.mToState);
            Archive.Serialize("Duration", Transition.mDuration);
            Archive.Serialize("Priority", Transition.mPriority);

            if (Archive.BeginObject("Rule")) {
                Transition.mRule.Serialize(Archive);
                Archive.EndObject();
            }

            Archive.EndObject();

            if (Transition.mId < 0 || Transition.mId == std::numeric_limits<Core::Int32>::max() || FindState(Transition.mFromState) == nullptr || FindState(Transition.mToState) == nullptr || !std::isfinite(Transition.mDuration) || Transition.mDuration < 0.0f || std::any_of(mTransitions.begin(), mTransitions.begin() + Index, [&Transition](const auto& Other) {
                return Other.mId == Transition.mId;
            })) {
                Archive.SetError("Invalid animation transition");
            } else {
                mNextTransitionId = std::max(mNextTransitionId, Transition.mId + 1);
            }
        }

        Archive.EndArray();

        if ((!mStates.empty() && FindState(mEntryState) == nullptr) || (mStates.empty() && mEntryState != -1)) {
            Archive.SetError("Invalid animation state machine entry");
        }
    }

}
