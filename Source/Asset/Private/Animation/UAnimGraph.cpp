/**
 * @file UAnimGraph.cpp
 * @brief 노드·링크·변수로 이루어진 애니메이션 그래프 에셋입니다.
 * @ingroup Asset
 */

#include "Asset/Animation/UAnimGraph.h"
#include "Core/Archive.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace Asset {

    /**
     * @brief 노드·링크·변수로 이루어진 애니메이션 그래프 에셋의 파일 내부 보조 구현입니다.
     */
    namespace {

        /**
         * @brief 직렬화하는 애니메이션 그래프 파일 형식 버전입니다.
         */
        constexpr Core::Uint32 AnimGraphFormatVersion{1};

        /**
         * @brief 노드 파라미터와 변수 Getter 연결을 저장하는 키 접미사입니다.
         */
        constexpr std::string_view BindingSuffix{".Binding"};
    }

    const Core::FString* FAnimGraphNode::FindParameter(const Core::FString& Name) const {
        const auto Found{std::find_if(mParameters.begin(), mParameters.end(), [&Name](const Asset::FAnimGraphParameter& Parameter) {
            return Parameter.mName == Name;
        })};

        return Found != mParameters.end() ? &Found->mValue : nullptr;
    }

    void FAnimGraphNode::SetParameter(const Core::FString& Name, const Core::FString& Value, Core::Int32 GetterId) {
        const auto Found{std::find_if(mParameters.begin(), mParameters.end(), [&Name](const Asset::FAnimGraphParameter& Parameter) {
            return Parameter.mName == Name;
        })};

        if (Value.empty()) {
            if (Found != mParameters.end()) {
                mParameters.erase(Found);
            }

            return;
        }

        if (Found != mParameters.end()) {
            Found->mValue = Value;
            Found->mGetterId = GetterId;
        } else {
            mParameters.push_back({Name, Value, GetterId});
        }
    }

    const Core::TArray<Asset::FAnimGraphNode>& UAnimGraph::GetNodes() const {
        return mNodes;
    }

    const Core::TArray<Asset::FAnimGraphLink>& UAnimGraph::GetLinks() const {
        return mLinks;
    }

    const Core::TArray<Asset::FAnimGraphVariable>& UAnimGraph::GetVariables() const {
        return mVariables;
    }

    Asset::FAnimGraphNode* UAnimGraph::FindNode(Core::Int32 Id) {
        const auto Found{std::find_if(mNodes.begin(), mNodes.end(), [Id](const Asset::FAnimGraphNode& Node) {
            return Node.mId == Id;
        })};

        return Found != mNodes.end() ? &*Found : nullptr;
    }

    const Asset::FAnimGraphNode* UAnimGraph::FindNode(Core::Int32 Id) const {
        return const_cast<UAnimGraph*>(this)->FindNode(Id);
    }

    Core::Int32 UAnimGraph::AddNode(const Core::FString& Type, const Core::FString& Name, float X, float Y) {
        if (mNodes.size() >= MaxNodes || mNextNodeId == std::numeric_limits<Core::Int32>::max() || !std::isfinite(X) || !std::isfinite(Y)) {
            return -2;
        }

        Asset::FAnimGraphNode Node{};

        Node.mId = mNextNodeId++;
        Node.mType = Type;
        Node.mName = Name;
        Node.mX = X;
        Node.mY = Y;

        if (Type == "StateMachine") {
            mStateMachines.try_emplace(Node.mId);
        }

        mNodes.push_back(std::move(Node));

        return mNodes.back().mId;
    }

    bool UAnimGraph::RemoveNode(Core::Int32 Id) {
        const auto Found{std::find_if(mNodes.begin(), mNodes.end(), [Id](const Asset::FAnimGraphNode& Node) {
            return Node.mId == Id;
        })};

        if (Found == mNodes.end()) {
            return false;
        }

        mStateMachines.erase(Id);
        mNodes.erase(Found);
        std::erase_if(mLinks, [Id](const Asset::FAnimGraphLink& Link) {
            return Link.mFromNode == Id || Link.mToNode == Id;
        });

        return true;
    }

    bool UAnimGraph::WouldCreateCycle(Core::Int32 FromNode, Core::Int32 ToNode) const {
        if (FromNode == ToNode) {
            return true;
        }

        // 포즈는 From에서 To로 흐릅니다. To의 출력을 따라가다 From에 닿으면 새 링크가 순환을 만듭니다.
        Core::TArray<Core::Int32> Pending{ToNode};
        Core::TArray<Core::Int32> Visited{};

        while (!Pending.empty()) {
            const Core::Int32 Current{Pending.back()};

            Pending.pop_back();

            if (Current == FromNode) {
                return true;
            }

            if (std::find(Visited.begin(), Visited.end(), Current) != Visited.end()) {
                continue;
            }

            Visited.push_back(Current);

            for (const Asset::FAnimGraphLink& Link : mLinks) {
                if (Link.mFromNode == Current) {
                    Pending.push_back(Link.mToNode);
                }
            }
        }

        return false;
    }

    bool UAnimGraph::Connect(Core::Int32 FromNode, Core::Int32 ToNode, Core::Int32 ToPin) {
        if (FindNode(FromNode) == nullptr || (ToNode != OutputNodeId && FindNode(ToNode) == nullptr) || ToPin < 0 || (ToNode == OutputNodeId && ToPin != 0) || WouldCreateCycle(FromNode, ToNode)) {
            return false;
        }

        Disconnect(ToNode, ToPin);
        mLinks.push_back({FromNode, ToNode, ToPin});

        return true;
    }

    bool UAnimGraph::Disconnect(Core::Int32 ToNode, Core::Int32 ToPin) {
        return std::erase_if(mLinks, [ToNode, ToPin](const Asset::FAnimGraphLink& Link) {
            return Link.mToNode == ToNode && Link.mToPin == ToPin;
        }) > 0;
    }

    Core::Int32 UAnimGraph::FindInput(Core::Int32 ToNode, Core::Int32 ToPin) const {
        for (const Asset::FAnimGraphLink& Link : mLinks) {
            if (Link.mToNode == ToNode && Link.mToPin == ToPin) {
                return Link.mFromNode;
            }
        }

        return OutputNodeId;
    }

    void UAnimGraph::SetNodePosition(Core::Int32 Id, float X, float Y) {
        if (!std::isfinite(X) || !std::isfinite(Y)) {
            return;
        }

        if (Id == OutputNodeId) {
            mOutputX = X;
            mOutputY = Y;
        } else if (Asset::FAnimGraphNode* Node{FindNode(Id)}) {
            Node->mX = X;
            Node->mY = Y;
        }
    }

    float UAnimGraph::GetOutputX() const {
        return mOutputX;
    }

    float UAnimGraph::GetOutputY() const {
        return mOutputY;
    }

    bool UAnimGraph::AddVariable(const Asset::FAnimGraphVariable& Variable) {
        if (Variable.mName.empty() || !std::isfinite(Variable.mDefault) || std::any_of(mVariables.begin(), mVariables.end(), [&Variable](const Asset::FAnimGraphVariable& Existing) {
            return Existing.mName == Variable.mName;
        })) {
            return false;
        }

        mVariables.push_back(Variable);
        UpdateChildVariables({}, {});

        return true;
    }

    bool UAnimGraph::SetVariable(Core::Int32 Index, const Asset::FAnimGraphVariable& Variable) {
        if (Index < 0 || static_cast<std::size_t>(Index) >= mVariables.size() || Variable.mName.empty() || !std::isfinite(Variable.mDefault)) {
            return false;
        }

        for (std::size_t Other{}; Other < mVariables.size(); ++Other) {
            if (Other != static_cast<std::size_t>(Index) && mVariables[Other].mName == Variable.mName) {
                return false;
            }
        }

        const Core::FString OldName{mVariables[static_cast<std::size_t>(Index)].mName};

        mVariables[static_cast<std::size_t>(Index)] = Variable;

        if (OldName != Variable.mName) {
            for (Asset::FAnimGraphVariableGetter& Getter : mVariableGetters) {
                if (Getter.mVariable == OldName) {
                    Getter.mVariable = Variable.mName;
                }
            }

            for (Asset::FAnimGraphNode& Node : mNodes) {
                for (Asset::FAnimGraphParameter& Parameter : Node.mParameters) {
                    if (Parameter.mName.ends_with(BindingSuffix) && Parameter.mValue == OldName) {
                        Parameter.mValue = Variable.mName;
                    }
                }
            }
        }

        UpdateChildVariables(OldName, Variable.mName);

        return true;
    }

    bool UAnimGraph::RemoveVariable(Core::Int32 Index) {
        if (Index < 0 || static_cast<std::size_t>(Index) >= mVariables.size()) {
            return false;
        }

        const Core::FString Name{mVariables[static_cast<std::size_t>(Index)].mName};

        std::erase_if(mVariableGetters, [&Name](const Asset::FAnimGraphVariableGetter& Getter) {
            return Getter.mVariable == Name;
        });

        for (Asset::FAnimGraphNode& Node : mNodes) {
            std::erase_if(Node.mParameters, [&Name](const Asset::FAnimGraphParameter& Parameter) {
                return Parameter.mName.ends_with(BindingSuffix) && Parameter.mValue == Name;
            });
        }

        mVariables.erase(mVariables.begin() + Index);
        UpdateChildVariables(Name, {});

        return true;
    }

    const Core::TArray<Asset::FAnimGraphVariableGetter>& UAnimGraph::GetVariableGetters() const {
        return mVariableGetters;
    }

    const Asset::FAnimGraphVariableGetter* UAnimGraph::FindVariableGetter(Core::Int32 Id) const {
        const auto Found{std::find_if(mVariableGetters.begin(), mVariableGetters.end(), [Id](const Asset::FAnimGraphVariableGetter& Getter) {
            return Getter.mId == Id;
        })};

        return Found == mVariableGetters.end() ? nullptr : &*Found;
    }

    const Asset::FAnimGraphVariableGetter* UAnimGraph::FindBoundGetter(const Asset::FAnimGraphNode& Node, const Core::FString& Parameter) const {
        const Core::FString Key{Parameter + ".Binding"};
        const auto Found{std::find_if(Node.mParameters.begin(), Node.mParameters.end(), [&Key](const Asset::FAnimGraphParameter& Binding) {
            return Binding.mName == Key;
        })};

        return Found == Node.mParameters.end() ? nullptr : ResolveGetter(*Found);
    }

    Core::Int32 UAnimGraph::AddVariableGetter(const Core::FString& Variable, float X, float Y) {
        if (mVariableGetters.size() >= MaxNodes || mNextGetterId == std::numeric_limits<Core::Int32>::max() || !std::isfinite(X) || !std::isfinite(Y) || std::none_of(mVariables.begin(), mVariables.end(), [&Variable](const Asset::FAnimGraphVariable& Entry) {
            return Entry.mName == Variable && Entry.mType == Asset::EAnimGraphVariableType::Float;
        })) {
            return -1;
        }

        const Core::Int32 Id{mNextGetterId++};

        mVariableGetters.push_back({Id, Variable, X, Y});

        return Id;
    }

    bool UAnimGraph::SetVariableGetterPosition(Core::Int32 Id, float X, float Y) {
        auto* Getter{const_cast<Asset::FAnimGraphVariableGetter*>(FindVariableGetter(Id))};

        if (Getter == nullptr || !std::isfinite(X) || !std::isfinite(Y)) {
            return false;
        }

        Getter->mX = X;
        Getter->mY = Y;

        return true;
    }

    bool UAnimGraph::RemoveVariableGetter(Core::Int32 Id) {
        const Asset::FAnimGraphVariableGetter* Getter{FindVariableGetter(Id)};

        if (Getter == nullptr) {
            return false;
        }

        for (Asset::FAnimGraphNode& Node : mNodes) {
            std::erase_if(Node.mParameters, [this, Getter](const Asset::FAnimGraphParameter& Parameter) {
                return Parameter.mName.ends_with(BindingSuffix) && Parameter.mValue == Getter->mVariable && ResolveGetter(Parameter) == Getter;
            });
        }

        mVariableGetters.erase(mVariableGetters.begin() + (Getter - mVariableGetters.data()));

        return true;
    }

    const Asset::FAnimGraphVariableGetter* UAnimGraph::ResolveGetter(const Asset::FAnimGraphParameter& Binding) const {
        if (const Asset::FAnimGraphVariableGetter* Getter{FindVariableGetter(Binding.mGetterId)}; Getter != nullptr && Getter->mVariable == Binding.mValue) {
            return Getter;
        }

        const auto Found{std::find_if(mVariableGetters.begin(), mVariableGetters.end(), [&Binding](const Asset::FAnimGraphVariableGetter& Getter) {
            return Getter.mVariable == Binding.mValue;
        })};

        return Found == mVariableGetters.end() ? nullptr : &*Found;
    }

    void UAnimGraph::CopyFrom(const Asset::UAnimGraph& Source) {
        if (this == &Source) {
            return;
        }

        mStateMachines.clear();

        for (const auto& [Id, Machine] : Source.mStateMachines) {
            mStateMachines[Id].CopyFrom(Machine);
        }

        mNodes = Source.mNodes;
        mLinks = Source.mLinks;
        mVariables = Source.mVariables;
        mVariableGetters = Source.mVariableGetters;
        mNextGetterId = Source.mNextGetterId;
        mNextNodeId = Source.mNextNodeId;
        mOutputX = Source.mOutputX;
        mOutputY = Source.mOutputY;
        mRevision = Core::FRevision::Next();
    }

    Asset::FAnimStateMachineData* UAnimGraph::FindStateMachine(Core::Int32 NodeId) {
        return const_cast<Asset::FAnimStateMachineData*>(std::as_const(*this).FindStateMachine(NodeId));
    }

    const Asset::FAnimStateMachineData* UAnimGraph::FindStateMachine(Core::Int32 NodeId) const {
        const auto Found{mStateMachines.find(NodeId)};

        return Found != mStateMachines.end() ? &Found->second : nullptr;
    }

    void UAnimGraph::SynchronizeVariables(const Core::TArray<Asset::FAnimGraphVariable>& Variables) {
        mVariables = Variables;
        UpdateChildVariables({}, {});
    }

    void UAnimGraph::UpdateChildVariables(const Core::FString& OldName, const Core::FString& NewName) {
        const auto HasFloat{[this](const Core::FString& Name) {
            return std::any_of(mVariables.begin(), mVariables.end(), [&Name](const auto& Variable) {
                return Variable.mName == Name && Variable.mType == Asset::EAnimGraphVariableType::Float;
            });
        }};

        std::erase_if(mVariableGetters, [&HasFloat](const auto& Getter) {
            return !HasFloat(Getter.mVariable);
        });

        for (auto& Node : mNodes) {
            std::erase_if(Node.mParameters, [&HasFloat](const auto& Parameter) {
                return Parameter.mName.ends_with(BindingSuffix) && !HasFloat(Parameter.mValue);
            });
        }

        for (auto& [Id, Machine] : mStateMachines) {
            for (const auto& State : Machine.GetStates()) {
                if (State.mGraph == nullptr) {
                    continue;
                }

                if (!OldName.empty()) {
                    const auto& Variables{State.mGraph->GetVariables()};
                    const auto Found{std::find_if(Variables.begin(), Variables.end(), [&OldName](const auto& Variable) {
                        return Variable.mName == OldName;
                    })};

                    if (Found != Variables.end()) {
                        const Core::Int32 Index{static_cast<Core::Int32>(Found - Variables.begin())};

                        if (NewName.empty()) {
                            State.mGraph->RemoveVariable(Index);
                        } else {
                            auto Variable{*Found};
                            Variable.mName = NewName;
                            State.mGraph->SetVariable(Index, Variable);
                        }
                    }
                }

                State.mGraph->SynchronizeVariables(mVariables);
            }

            for (const auto& Transition : Machine.GetTransitions()) {
                Machine.FindTransition(Transition.mId)->mRule.RenameVariable(OldName, NewName);
            }
        }
    }

    void UAnimGraph::SerializeStateMachines(Core::FArchive& Archive) {
        if (Archive.HasError()) {
            return;
        }

        if (Archive.IsLoading()) {
            mStateMachines.clear();

            if (!Archive.HasField("StateMachines")) {
                if (std::any_of(mNodes.begin(), mNodes.end(), [](const auto& Node) {
                    return Node.mType == "StateMachine";
                })) {
                    Archive.SetError("Missing animation state machine data");
                }

                return;
            }
        }

        static thread_local Core::Uint32 Depth{};

        if (Depth >= 16) {
            Archive.SetError("Animation state machine nesting limit exceeded");

            return;
        }

        Core::Uint32 Count{static_cast<Core::Uint32>(mStateMachines.size())};

        if (!Archive.BeginArray("StateMachines", Count)) {
            return;
        }

        if (Count > MaxNodes) {
            Archive.SetError("Too many animation state machines");
        }

        Core::TArray<Core::Int32> Ids{};

        if (!Archive.IsLoading()) {
            for (const auto& [Id, Machine] : mStateMachines) {
                Ids.push_back(Id);
            }

            std::sort(Ids.begin(), Ids.end());
        }

        ++Depth;

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            Core::Int32 Id{Archive.IsLoading() ? -1 : Ids[Index]};
            Archive.Serialize("Node", Id);
            const auto* Node{FindNode(Id)};

            if (Node == nullptr || Node->mType != "StateMachine" || (Archive.IsLoading() && mStateMachines.contains(Id))) {
                Archive.SetError("Invalid or duplicate state machine node");
            } else {
                mStateMachines[Id].Serialize(Archive);
            }

            Archive.EndObject();
        }

        --Depth;
        Archive.EndArray();

        if (!Archive.HasError()) {
            for (const auto& Node : mNodes) {
                if (Node.mType == "StateMachine" && !mStateMachines.contains(Node.mId)) {
                    Archive.SetError("Missing animation state machine data");
                }
            }
        }
    }

    Core::FRevision UAnimGraph::GetRevision() const {
        return mRevision;
    }

    void UAnimGraph::SerializeVariableGetters(Core::FArchive& Archive) {
        Core::Uint32 Count{static_cast<Core::Uint32>(mVariableGetters.size())};

        if (!Archive.BeginArray("VariableGetters", Count)) {
            return;
        }

        if (Archive.IsLoading() && Count > MaxNodes) {
            Archive.SetError("Too many anim graph variable getters");
        }

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            Asset::FAnimGraphVariableGetter Getter{Archive.IsLoading() ? Asset::FAnimGraphVariableGetter{} : mVariableGetters[Index]};

            Archive.Serialize("Id", Getter.mId);
            Archive.Serialize("Variable", Getter.mVariable);
            Archive.Serialize("X", Getter.mX);
            Archive.Serialize("Y", Getter.mY);
            Archive.EndObject();

            if (Archive.IsLoading() && !Archive.HasError()) {
                const bool Valid{Getter.mId >= 0 && Getter.mId < std::numeric_limits<Core::Int32>::max() && FindVariableGetter(Getter.mId) == nullptr && !Getter.mVariable.empty() && std::isfinite(Getter.mX) && std::isfinite(Getter.mY) && std::any_of(mVariables.begin(), mVariables.end(), [&Getter](const Asset::FAnimGraphVariable& Variable) {
                    return Variable.mName == Getter.mVariable;
                })};

                if (!Valid) {
                    Archive.SetError("Invalid anim graph variable getter");
                } else {
                    mNextGetterId = std::max(mNextGetterId, Getter.mId + 1);
                    mVariableGetters.push_back(std::move(Getter));
                }
            }
        }

        Archive.EndArray();
    }

    void UAnimGraph::Serialize(Core::FArchive& Archive) {
        Core::Uint32 Version{AnimGraphFormatVersion};

        Archive.Serialize("FormatVersion", Version);

        if (Archive.HasError()) {
            return;
        }

        if (Version != AnimGraphFormatVersion) {
            Archive.SetError("Unsupported anim graph version");

            return;
        }

        Archive.Serialize("OutputX", mOutputX);
        Archive.Serialize("OutputY", mOutputY);

        const bool LoadLegacyGetters{Archive.IsLoading() && !Archive.HasField("VariableGetters")};
        Core::Uint32 Count{static_cast<Core::Uint32>(mVariables.size())};

        if (!Archive.BeginArray("Variables", Count)) {
            return;
        }

        if (Archive.IsLoading()) {
            mVariables.assign(std::min<Core::Uint32>(Count, MaxNodes), Asset::FAnimGraphVariable{});
            mVariableGetters.clear();
            mNextGetterId = 0;
        }

        for (Core::Uint32 Index{}; Index < mVariables.size() && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            Asset::FAnimGraphVariable& Variable{mVariables[Index]};
            Core::Uint32 Source{static_cast<Core::Uint32>(Variable.mSource)};

            Archive.Serialize("Name", Variable.mName);
            Archive.Serialize("Default", Variable.mDefault);
            Archive.Serialize("Source", Source);

            if (!Archive.IsLoading() || Archive.HasField("Type")) {
                Core::Uint32 Type{static_cast<Core::Uint32>(Variable.mType)};
                Archive.Serialize("Type", Type);
                Archive.Serialize("BoolDefault", Variable.mBoolDefault);

                if (Type > static_cast<Core::Uint32>(Asset::EAnimGraphVariableType::Bool)) {
                    Archive.SetError("Invalid anim graph variable type");
                } else {
                    Variable.mType = static_cast<Asset::EAnimGraphVariableType>(Type);
                }
            }

            if (LoadLegacyGetters && Archive.HasField("GetterX") && Archive.HasField("GetterY")) {
                float X{};
                float Y{};

                Archive.Serialize("GetterX", X);
                Archive.Serialize("GetterY", Y);

                if (!Archive.HasError()) {
                    AddVariableGetter(Variable.mName, X, Y);
                }
            }

            if (Archive.IsLoading()) {
                Variable.mSource = Source == static_cast<Core::Uint32>(Asset::EAnimGraphVariableSource::OwnerSpeed) ? Asset::EAnimGraphVariableSource::OwnerSpeed : Asset::EAnimGraphVariableSource::Manual;
            }

            Archive.EndObject();
        }

        Archive.EndArray();

        if (!LoadLegacyGetters && !Archive.HasError()) {
            SerializeVariableGetters(Archive);
        }

        Count = static_cast<Core::Uint32>(mNodes.size());

        if (Archive.HasError() || !Archive.BeginArray("Nodes", Count)) {
            return;
        }

        if (Archive.IsLoading()) {
            if (Count > MaxNodes) {
                Archive.SetError("Too many anim graph nodes");
                Archive.EndArray();

                return;
            }

            mNodes.assign(Count, Asset::FAnimGraphNode{});
        }

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            Asset::FAnimGraphNode& Node{mNodes[Index]};

            Archive.Serialize("Id", Node.mId);
            Archive.Serialize("Type", Node.mType);
            Archive.Serialize("Name", Node.mName);
            Archive.Serialize("X", Node.mX);
            Archive.Serialize("Y", Node.mY);

            Core::Uint32 ParameterCount{static_cast<Core::Uint32>(Node.mParameters.size())};

            if (Archive.BeginArray("Parameters", ParameterCount)) {
                if (Archive.IsLoading()) {
                    Node.mParameters.assign(std::min<Core::Uint32>(ParameterCount, 64), Asset::FAnimGraphParameter{});
                }

                for (Core::Uint32 Parameter{}; Parameter < Node.mParameters.size() && !Archive.HasError(); ++Parameter) {
                    if (!Archive.BeginObject(std::to_string(Parameter))) {
                        break;
                    }

                    Asset::FAnimGraphParameter& Value{Node.mParameters[Parameter]};

                    Archive.Serialize("Name", Value.mName);
                    Archive.Serialize("Value", Value.mValue);

                    if (Archive.IsLoading() ? Archive.HasField("Getter") : Value.mGetterId >= 0) {
                        Archive.Serialize("Getter", Value.mGetterId);
                    }

                    Archive.EndObject();
                }

                Archive.EndArray();
            }

            Archive.EndObject();
        }

        Archive.EndArray();

        Count = static_cast<Core::Uint32>(mLinks.size());

        if (Archive.HasError() || !Archive.BeginArray("Links", Count)) {
            return;
        }

        if (Archive.IsLoading()) {
            mLinks.assign(std::min<Core::Uint32>(Count, MaxNodes * 4), Asset::FAnimGraphLink{});
        }

        for (Core::Uint32 Index{}; Index < mLinks.size() && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            Archive.Serialize("From", mLinks[Index].mFromNode);
            Archive.Serialize("To", mLinks[Index].mToNode);
            Archive.Serialize("Pin", mLinks[Index].mToPin);
            Archive.EndObject();
        }

        Archive.EndArray();

        SerializeStateMachines(Archive);

        if (!Archive.IsLoading() || Archive.HasError()) {
            return;
        }

        // 읽은 링크 중 없는 노드를 가리키거나 핀을 겹쳐 쓰는 링크는 버리고, 다음 노드 식별자를 다시 계산합니다.
        Core::TArray<Asset::FAnimGraphLink> Links{std::move(mLinks)};

        mLinks.clear();
        mNextNodeId = 0;

        for (std::size_t Index{}; Index < mNodes.size(); ++Index) {
            const Core::Int32 Id{mNodes[Index].mId};

            if (Id < 0 || Id == std::numeric_limits<Core::Int32>::max() || std::any_of(mNodes.begin(), mNodes.begin() + static_cast<std::ptrdiff_t>(Index), [Id](const Asset::FAnimGraphNode& Node) {
                return Node.mId == Id;
            })) {
                Archive.SetError("Invalid or duplicate anim graph node id");

                return;
            }

            mNextNodeId = std::max(mNextNodeId, Id + 1);
        }

        for (const Asset::FAnimGraphLink& Link : Links) {
            Connect(Link.mFromNode, Link.mToNode, Link.mToPin);
        }

        UpdateChildVariables({}, {});
    }

}
