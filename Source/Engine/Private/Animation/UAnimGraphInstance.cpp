/**
 * @file UAnimGraphInstance.cpp
 * @brief 에디터에서 만든 UAnimGraph 에셋을 실행합니다.
 * @ingroup Engine
 */

#include "Engine/Animation/UAnimGraphInstance.h"
#include "Asset/FAssetManager.h"
#include "Core/FilePath.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/UWorld.h"
#include "Engine/Animation/FAnimGraphNodeRegistry.h"
#include "Engine/Animation/FAnimNode_AssetPlayerBase.h"
#include "Engine/Animation/FAnimNode_Base.h"
#include "Engine/Animation/FAnimValueNode.h"
#include "Engine/Animation/FAnimNode_StateMachine.h"
#include "Engine/Component/USkeletalMeshComponent.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace Engine {

    UAnimGraphInstance::UAnimGraphInstance() = default;

    UAnimGraphInstance::~UAnimGraphInstance() = default;

    const Core::FString& UAnimGraphInstance::GetAnimGraph() const {
        return mAnimGraph;
    }

    bool UAnimGraphInstance::SetAnimGraph(const Core::FString& Path) {
        const std::filesystem::path AssetPath{Core::FilePath::FromUtf8(Path)};

        if (Path.find('\0') != Core::FString::npos || (!Path.empty() && (AssetPath.has_root_path() || AssetPath.extension() != L".masset"))) {
            return false;
        }

        mAnimGraph = Path;

        return true;
    }

    bool UAnimGraphInstance::SetFloatVariable(const Core::FString& Name, float Value) {
        for (Engine::FAnimGraphRuntimeVariable& Variable : mVariables) {
            if (Variable.mName == Name && Variable.mType == Asset::EAnimGraphVariableType::Float) {
                Variable.mValue = std::isfinite(Value) ? Value : 0.0f;

                return true;
            }
        }

        return false;
    }

    float UAnimGraphInstance::GetFloatVariable(const Core::FString& Name) const {
        for (const Engine::FAnimGraphRuntimeVariable& Variable : mVariables) {
            if (Variable.mName == Name && Variable.mType == Asset::EAnimGraphVariableType::Float) {
                return Variable.mValue;
            }
        }

        return 0.0f;
    }

    bool UAnimGraphInstance::SetBoolVariable(const Core::FString& Name, bool Value) {
        for (auto& Variable : mVariables) {
            if (Variable.mName == Name && Variable.mType == Asset::EAnimGraphVariableType::Bool) {
                Variable.mBoolValue = Value;

                return true;
            }
        }

        return false;
    }

    bool UAnimGraphInstance::GetBoolVariable(const Core::FString& Name) const {
        for (const auto& Variable : mVariables) {
            if (Variable.mName == Name && Variable.mType == Asset::EAnimGraphVariableType::Bool) {
                return Variable.mBoolValue;
            }
        }

        return false;
    }

    bool UAnimGraphInstance::HasGraphVariable(const Core::FString& Name, bool IsBool) const {
        return std::any_of(mVariables.begin(), mVariables.end(), [&Name, IsBool](const auto& Variable) {
            return Variable.mName == Name && (Variable.mType == Asset::EAnimGraphVariableType::Bool) == IsBool;
        });
    }

    Asset::EAnimGraphVariableSource UAnimGraphInstance::GetVariableSource(const Core::FString& Name) const {
        const auto Variable{std::find_if(mVariables.begin(), mVariables.end(), [&Name](const Engine::FAnimGraphRuntimeVariable& Candidate) {
            return Candidate.mName == Name;
        })};

        return Variable != mVariables.end() ? Variable->mSource : Asset::EAnimGraphVariableSource::Manual;
    }

    Engine::FAnimNode_AssetPlayerBase* UAnimGraphInstance::FindAssetPlayer(const Core::FString& Name) const {
        if (Name.empty()) {
            return nullptr;
        }

        Engine::FAnimNode_AssetPlayerBase* Player{};

        for (const std::unique_ptr<Engine::FAnimNode_Base>& Node : mNodes) {
            if (Node == nullptr || Node->mName != Name) {
                continue;
            }

            auto* Candidate{dynamic_cast<Engine::FAnimNode_AssetPlayerBase*>(Node.get())};

            if (Candidate != nullptr) {
                if (Player != nullptr) {
                    return nullptr;
                }

                Player = Candidate;
            }
        }

        return Player;
    }

    void UAnimGraphInstance::GetDebugVariables(Core::TArray<Engine::FAnimNodeValue>& OutValues) {
        for (Engine::FAnimGraphRuntimeVariable& Variable : mVariables) {
            Engine::FAnimNodeValue Value{};

            Value.mName = Variable.mName;
            if (Variable.mType == Asset::EAnimGraphVariableType::Bool) {
                Value.mValue = &Variable.mBoolValue;
            } else {
                Value.mValue = Engine::FAnimNodeFloatValue{&Variable.mValue};
            }
            OutValues.push_back(std::move(Value));
        }
    }

    bool UAnimGraphInstance::GetNodeLayout(const Engine::FAnimNode_Base* Node, float& X, float& Y) const {
        if (mSourceGraph == nullptr || mGraphRevision != mSourceGraph->GetRevision()) {
            return false;
        }

        if (Node == nullptr) {
            X = mSourceGraph->GetOutputX();
            Y = mSourceGraph->GetOutputY();

            return std::any_of(mNodes.begin(), mNodes.end(), [](const auto& Candidate) {
                return Candidate != nullptr;
            });
        }

        const Asset::FAnimGraphNode* Source{GetSourceNode(Node)};

        if (Source == nullptr) {
            return false;
        }

        X = Source->mX;
        Y = Source->mY;

        const auto Found{std::find_if(mNodes.begin(), mNodes.end(), [Node](const auto& Candidate) {
            return Candidate.get() == Node;
        })};

        return Found != mNodes.end() && mNodeGraphs[static_cast<std::size_t>(Found - mNodes.begin())] == mSourceGraph;
    }

    const Asset::FAnimGraphNode* UAnimGraphInstance::GetSourceNode(const Engine::FAnimNode_Base* Node) const {
        if (Node == nullptr || mSourceGraph == nullptr || mGraphRevision != mSourceGraph->GetRevision()) {
            return nullptr;
        }

        const auto Found{std::find_if(mNodes.begin(), mNodes.end(), [Node](const auto& Candidate) {
            return Candidate.get() == Node;
        })};

        return Found != mNodes.end() ? mNodeSources[static_cast<std::size_t>(Found - mNodes.begin())] : nullptr;
    }

    const Asset::FAnimGraphVariableGetter* UAnimGraphInstance::GetSourceGetter(const Engine::FAnimValueNode_Base* Node) const {
        if (Node == nullptr || mSourceGraph == nullptr || mGraphRevision != mSourceGraph->GetRevision()) {
            return nullptr;
        }

        const auto Found{std::find_if(mValueNodes.begin(), mValueNodes.end(), [Node](const auto& Candidate) {
            return Candidate.get() == Node;
        })};

        if (Found == mValueNodes.end()) {
            return nullptr;
        }

        const std::size_t Index{static_cast<std::size_t>(Found - mValueNodes.begin())};
        return Index < mGetterSources.size() ? mGetterSources[Index] : nullptr;
    }

    Core::Uint64 UAnimGraphInstance::GetNodeKey(const Engine::FAnimNode_Base* Node) const {
        const auto Found{std::find_if(mNodes.begin(), mNodes.end(), [Node](const auto& Candidate) {
            return Candidate.get() == Node;
        })};

        return Found != mNodes.end() ? static_cast<Core::Uint64>(Found - mNodes.begin()) + 1 : 0;
    }

    Core::Uint64 UAnimGraphInstance::GetValueNodeKey(const Engine::FAnimValueNode_Base* Node) const {
        const auto Found{std::find_if(mValueNodes.begin(), mValueNodes.end(), [Node](const auto& Candidate) {
            return Candidate.get() == Node;
        })};

        return Found != mValueNodes.end() ? static_cast<Core::Uint64>(Found - mValueNodes.begin()) + 1 : 0;
    }

    void UAnimGraphInstance::NativeInitializeAnimation() {
        mSourceGraph = nullptr;
        mGraphRevision = {};
        mRootNode = nullptr;
        mNodes.clear();
        mValueNodes.clear();
        mNodeSources.clear();
        mNodeGraphs.clear();
        mGetterSources.clear();
        mVariables.clear();

        Engine::USkeletalMeshComponent* Owner{GetOwningComponent()};
        Asset::FAssetManager* Assets{Owner != nullptr && Owner->GetWorld() != nullptr ? Owner->GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || mAnimGraph.empty()) {
            return;
        }

        Core::FString Error{};
        const Asset::UAnimGraph* Graph{Assets->LoadAnimGraph(Core::FilePath::FromUtf8(mAnimGraph), Error)};

        if (Graph == nullptr) {
            Core::ErrorHandler::Report("Animation", "Cannot load anim graph '" + mAnimGraph + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);

            return;
        }

        mSourceGraph = Graph;
        mGraphRevision = Graph->GetRevision();

        mVariables.reserve(Graph->GetVariables().size());

        for (const Asset::FAnimGraphVariable& Variable : Graph->GetVariables()) {
            mVariables.push_back({Variable.mName, Variable.mDefault, Variable.mSource, Variable.mType, Variable.mBoolDefault});
        }

        Engine::FAnimGraphNodeBuildContext Context{};

        Context.mLoadSequence = [this](const Core::FString& Path) {
            return LoadAnimSequence(Path);
        };
        Context.mLoadBlendSpace = [this](const Core::FString& Path) {
            return LoadBlendSpace(Path);
        };
        mRootNode = BuildGraph(*Graph, Context, 0);
    }

    Engine::FAnimNode_Base* UAnimGraphInstance::BuildGraph(const Asset::UAnimGraph& Source, const Engine::FAnimGraphNodeBuildContext& BaseContext, Core::Uint32 Depth) {
        if (Depth >= 16) {
            return nullptr;
        }

        const Asset::UAnimGraph* Graph{&Source};
        Engine::FAnimGraphNodeBuildContext Context{BaseContext};
        Engine::FAnimNode_Base* Root{};
        const auto& Getters{Graph->GetVariableGetters()};
        const std::size_t GetterOffset{mValueNodes.size()};

        for (const Asset::FAnimGraphVariableGetter& Data : Getters) {
            auto Getter{std::make_unique<Engine::FAnimValueNode_GetFloat>()};
            Getter->mVariableName = Data.mVariable;
            mValueNodes.push_back(std::move(Getter));
            mGetterSources.push_back(&Data);
        }

        // 에셋의 노드마다 실행 노드를 만듭니다. 알 수 없는 종류는 건너뛰고, 그 노드에 이어진 링크도 무시됩니다.
        Core::TMap<Core::Int32, Engine::FAnimNode_Base*> NodesById{};

        const auto& SourceNodes{Graph->GetNodes()};
        Core::TArray<Engine::FAnimNode_Base*> LocalNodes{};
        LocalNodes.resize(SourceNodes.size());

        for (std::size_t Index{}; Index < SourceNodes.size(); ++Index) {
            const Asset::FAnimGraphNode& Data{SourceNodes[Index]};
            const Engine::FAnimGraphNodeType* Type{Engine::FAnimGraphNodeRegistry::Find(Data.mType)};

            if (Type == nullptr || !Type->mCreate) {
                Core::ErrorHandler::Report("Animation", "Unknown node type '" + Data.mType + "' in anim graph '" + mAnimGraph + "'", Core::ErrorHandler::EErrorLevel::Warning);

                continue;
            }

            Context.mNode = &Data;
            Context.mParameters = &Type->mParameters;

            std::unique_ptr<Engine::FAnimNode_Base> Node{Type->mCreate(Context)};

            if (Node == nullptr) {
                continue;
            }

            Node->mName = Data.mName;
            NodesById.emplace(Data.mId, Node.get());
            LocalNodes[Index] = Node.get();
            mNodes.push_back(std::move(Node));
            mNodeSources.push_back(&Data);
            mNodeGraphs.push_back(Graph);

            if (auto* Machine{dynamic_cast<Engine::FAnimNode_StateMachine*>(LocalNodes[Index])}) {
                const auto* Definition{Graph->FindStateMachine(Data.mId)};

                if (Definition != nullptr) {
                    Machine->Configure(*Definition);

                    for (const auto& State : Definition->GetStates()) {
                        Machine->AddState(State, State.mGraph != nullptr ? BuildGraph(*State.mGraph, BaseContext, Depth + 1) : nullptr);
                    }
                }
            }
        }

        // 링크대로 포즈 입력을 연결합니다. 에셋은 순환 연결과 핀 중복을 허용하지 않습니다.
        Core::TMap<Core::FString, Engine::FAnimValueNode_Base*> ImplicitGetters{};

        for (std::size_t Index{}; Index < LocalNodes.size(); ++Index) {
            if (LocalNodes[Index] == nullptr) {
                continue;
            }

            Core::TArray<Engine::FAnimNodeValueInput> Inputs{};
            LocalNodes[Index]->GetValueInputs(Inputs);

            for (const Engine::FAnimNodeValueInput& Input : Inputs) {
                const Core::FString* VariableName{SourceNodes[Index].FindParameter(Input.mName + ".Binding")};

                if (VariableName == nullptr || std::none_of(mVariables.begin(), mVariables.end(), [VariableName](const Engine::FAnimGraphRuntimeVariable& Variable) {
                    return Variable.mName == *VariableName && Variable.mType == Asset::EAnimGraphVariableType::Float;
                })) {
                    continue;
                }

                const Asset::FAnimGraphVariableGetter* SourceGetter{Graph->FindBoundGetter(SourceNodes[Index], Input.mName)};

                if (SourceGetter != nullptr) {
                    const std::size_t GetterIndex{static_cast<std::size_t>(SourceGetter - Getters.data())};
                    Input.mLink->SetLinkedNode(mValueNodes[GetterOffset + GetterIndex].get());
                    continue;
                }

                Engine::FAnimValueNode_Base*& Getter{ImplicitGetters[*VariableName]};

                if (Getter == nullptr) {
                    auto Node{std::make_unique<Engine::FAnimValueNode_GetFloat>()};
                    Node->mVariableName = *VariableName;
                    Getter = Node.get();
                    mValueNodes.push_back(std::move(Node));
                    mGetterSources.push_back(nullptr);
                }

                Input.mLink->SetLinkedNode(Getter);
            }
        }

        for (const Asset::FAnimGraphLink& Link : Graph->GetLinks()) {
            const auto From{NodesById.find(Link.mFromNode)};

            if (From == NodesById.end()) {
                continue;
            }

            if (Link.mToNode == Asset::UAnimGraph::OutputNodeId) {
                Root = From->second;

                continue;
            }

            const auto To{NodesById.find(Link.mToNode)};

            if (To == NodesById.end()) {
                continue;
            }

            Core::TArray<Engine::FAnimNodePoseInput> Inputs{};

            To->second->GetPoseInputs(Inputs);

            if (Link.mToPin >= 0 && static_cast<std::size_t>(Link.mToPin) < Inputs.size()) {
                Inputs[static_cast<std::size_t>(Link.mToPin)].mLink->mLinkedNode = From->second;
            }
        }
        return Root;
    }

    void UAnimGraphInstance::NativeUpdateAnimation(float) {
        Engine::USkeletalMeshComponent* Owner{GetOwningComponent()};
        const Engine::UWorld* World{Owner != nullptr ? Owner->GetWorld() : nullptr};

        if (World != nullptr && World->GetWorldType() == Engine::EWorldType::PIE && mSourceGraph != nullptr && mGraphRevision != mSourceGraph->GetRevision()) {
            NativeInitializeAnimation();

            if (mRootNode != nullptr) {
                mRootNode->Initialize(*this);
            }
        }

        const Core::FVector& Velocity{GetOwnerVelocity()};
        const float Speed{std::sqrt(Velocity.x * Velocity.x + Velocity.y * Velocity.y)};

        for (Engine::FAnimGraphRuntimeVariable& Variable : mVariables) {
            if (Variable.mSource == Asset::EAnimGraphVariableSource::OwnerSpeed && Variable.mType == Asset::EAnimGraphVariableType::Float) {
                Variable.mValue = Speed;
            }
        }
    }

}
