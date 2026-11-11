#pragma once

#include "Core/Types.h"

#include <memory>

namespace Core {
    class FArchive;
}

namespace Asset {

    class UAnimGraph;

    enum class EAnimRuleNodeType : Core::Uint8 {
        Float,
        Bool,
        GetFloat,
        GetBool,
        Greater,
        GreaterEqual,
        Less,
        LessEqual,
        Equal,
        NotEqual,
        And,
        Or,
        Not,
        StateTime,
        TimeRemaining,
        Finished,
        Count
    };

    struct FAnimRuleNode {
    public:
        Core::Int32 mId{};
        Asset::EAnimRuleNodeType mType{};
        Core::Int32 mInputA{-1};
        Core::Int32 mInputB{-1};
        Core::FString mName{};
        float mValue{};
        bool mBoolValue{};
        float mX{};
        float mY{};
    };

    class FAnimTransitionGraph {
    public:
        const Core::TArray<Asset::FAnimRuleNode>& GetNodes() const;
        Asset::FAnimRuleNode* FindNode(Core::Int32 Id);
        const Asset::FAnimRuleNode* FindNode(Core::Int32 Id) const;
        Core::Int32 AddNode(Asset::EAnimRuleNodeType Type, float X, float Y);
        bool RemoveNode(Core::Int32 Id);
        bool Connect(Core::Int32 From, Core::Int32 To, Core::Int32 Pin);
        bool Disconnect(Core::Int32 To, Core::Int32 Pin);
        bool SetResult(Core::Int32 Id);
        Core::Int32 GetResult() const;
        bool Validate() const;
        void RenameVariable(const Core::FString& OldName, const Core::FString& NewName);
        void Serialize(Core::FArchive& Archive);

        static bool IsBool(Asset::EAnimRuleNodeType Type);
        static Core::Int32 GetInputCount(Asset::EAnimRuleNodeType Type);
        static bool IsBoolInput(Asset::EAnimRuleNodeType Type);
        static const char* GetTitle(Asset::EAnimRuleNodeType Type);

    private:
        bool Reaches(Core::Int32 From, Core::Int32 To) const;

    private:
        Core::TArray<Asset::FAnimRuleNode> mNodes{};
        Core::Int32 mNextId{};
        Core::Int32 mResult{-1};
    };

    struct FAnimStateData {
    public:
        Core::Int32 mId{};
        Core::FString mName{};
        float mX{};
        float mY{};
        bool mResetOnEntry{true};
        std::shared_ptr<Asset::UAnimGraph> mGraph{};
    };

    struct FAnimTransitionData {
    public:
        Core::Int32 mId{};
        Core::Int32 mFromState{};
        Core::Int32 mToState{};
        float mDuration{0.2f};
        Core::Int32 mPriority{};
        Asset::FAnimTransitionGraph mRule{};
    };

    class FAnimStateMachineData {
    public:
        static constexpr Core::Uint32 MaxStates{254};

        const Core::TArray<Asset::FAnimStateData>& GetStates() const;
        const Core::TArray<Asset::FAnimTransitionData>& GetTransitions() const;
        Asset::FAnimStateData* FindState(Core::Int32 Id);
        const Asset::FAnimStateData* FindState(Core::Int32 Id) const;
        Asset::FAnimTransitionData* FindTransition(Core::Int32 Id);
        const Asset::FAnimTransitionData* FindTransition(Core::Int32 Id) const;
        Core::Int32 AddState(const Core::FString& Name, float X, float Y, const Asset::UAnimGraph& Owner);
        bool RemoveState(Core::Int32 Id);
        Core::Int32 AddTransition(Core::Int32 From, Core::Int32 To);
        bool RemoveTransition(Core::Int32 Id);
        bool SetEntryState(Core::Int32 Id);
        Core::Int32 GetEntryState() const;
        void CopyFrom(const Asset::FAnimStateMachineData& Source);
        void Serialize(Core::FArchive& Archive);

    private:
        Core::TArray<Asset::FAnimStateData> mStates{};
        Core::TArray<Asset::FAnimTransitionData> mTransitions{};
        Core::Int32 mEntryState{-1};
        Core::Int32 mNextStateId{};
        Core::Int32 mNextTransitionId{};
    };

}
