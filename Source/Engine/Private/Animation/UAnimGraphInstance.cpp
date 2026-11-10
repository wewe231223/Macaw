#include "Engine/Animation/UAnimGraphInstance.h"
#include "Asset/FAssetManager.h"
#include "Core/FilePath.h"
#include "Core/Base/ErrorHandler.h"
#include "Engine/UWorld.h"
#include "Engine/Animation/FAnimGraphNodeRegistry.h"
#include "Engine/Animation/FAnimNode_AssetPlayerBase.h"
#include "Engine/Animation/FAnimNode_Base.h"
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
            if (Variable.mName == Name) {
                Variable.mValue = std::isfinite(Value) ? Value : 0.0f;

                return true;
            }
        }

        return false;
    }

    float UAnimGraphInstance::GetFloatVariable(const Core::FString& Name) const {
        for (const Engine::FAnimGraphRuntimeVariable& Variable : mVariables) {
            if (Variable.mName == Name) {
                return Variable.mValue;
            }
        }

        return 0.0f;
    }

    Engine::FAnimNode_AssetPlayerBase* UAnimGraphInstance::FindAssetPlayer(Core::Int32 NodeId) const {
        const auto Node{std::find_if(mNodes.begin(), mNodes.end(), [NodeId](const std::unique_ptr<Engine::FAnimNode_Base>& Candidate) {
            return Candidate->mGraphNodeId == NodeId;
        })};

        return Node != mNodes.end() ? dynamic_cast<Engine::FAnimNode_AssetPlayerBase*>(Node->get()) : nullptr;
    }

    void UAnimGraphInstance::GetDebugVariables(Core::TArray<Engine::FAnimNodeValue>& OutValues) {
        for (Engine::FAnimGraphRuntimeVariable& Variable : mVariables) {
            Engine::FAnimNodeValue Value{};

            Value.mName = Variable.mName;
            Value.mType = Engine::EAnimNodeValueType::Float;
            Value.mFloat = &Variable.mValue;
            Value.mBinding = Variable.mSource == Asset::EAnimGraphVariableSource::OwnerSpeed ? Core::FString{"Owner Speed"} : Core::FString{};
            OutValues.push_back(std::move(Value));
        }
    }

    bool UAnimGraphInstance::GetNodeLayout(const Engine::FAnimNode_Base* Node, float& X, float& Y) const {
        if (Node == nullptr) {
            X = mOutputX;
            Y = mOutputY;

            return !mNodes.empty();
        }

        for (std::size_t Index{}; Index < mNodes.size(); ++Index) {
            if (mNodes[Index].get() == Node) {
                X = mNodePositions[Index * 2];
                Y = mNodePositions[Index * 2 + 1];

                return true;
            }
        }

        return false;
    }

    const Asset::FAnimGraphVariableGetter* UAnimGraphInstance::GetVariableGetter(Core::Int32 NodeId, const Core::FString& Parameter) const {
        const Asset::FAnimGraphNode* Node{mSourceGraph != nullptr && mGraphRevision == mSourceGraph->GetRevision() ? mSourceGraph->FindNode(NodeId) : nullptr};

        return Node != nullptr ? mSourceGraph->FindBoundGetter(*Node, Parameter) : nullptr;
    }

    void UAnimGraphInstance::NativeInitializeAnimation() {
        mSourceGraph = nullptr;
        mGraphRevision = {};
        mRootNode = nullptr;
        mNodes.clear();
        mNodePositions.clear();
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
        mOutputX = Graph->GetOutputX();
        mOutputY = Graph->GetOutputY();

        // 노드 바인딩이 변수 주소를 가리키므로 변수 배열은 여기서 한 번에 채우고 다시 크기를 바꾸지 않습니다.
        mVariables.reserve(Graph->GetVariables().size());

        for (const Asset::FAnimGraphVariable& Variable : Graph->GetVariables()) {
            mVariables.push_back({Variable.mName, Variable.mDefault, Variable.mSource});
        }

        Engine::FAnimGraphNodeBuildContext Context{};

        Context.mLoadSequence = [this](const Core::FString& Path) {
            return LoadAnimSequence(Path);
        };
        Context.mLoadBlendSpace = [this](const Core::FString& Path) {
            return LoadBlendSpace(Path);
        };
        Context.mFindVariable = [this](const Core::FString& Name) -> const float* {
            for (const Engine::FAnimGraphRuntimeVariable& Variable : mVariables) {
                if (Variable.mName == Name) {
                    return &Variable.mValue;
                }
            }

            return nullptr;
        };

        // 에셋의 노드마다 실행 노드를 만듭니다. 알 수 없는 종류는 건너뛰고, 그 노드에 이어진 링크도 무시됩니다.
        Core::TMap<Core::Int32, Engine::FAnimNode_Base*> NodesById{};

        for (const Asset::FAnimGraphNode& Data : Graph->GetNodes()) {
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
            Node->mGraphNodeId = Data.mId;
            NodesById.emplace(Data.mId, Node.get());
            mNodePositions.push_back(Data.mX);
            mNodePositions.push_back(Data.mY);
            mNodes.push_back(std::move(Node));
        }

        // 링크대로 포즈 입력을 연결합니다. 에셋은 순환 연결과 핀 중복을 허용하지 않습니다.
        for (const Asset::FAnimGraphLink& Link : Graph->GetLinks()) {
            const auto From{NodesById.find(Link.mFromNode)};

            if (From == NodesById.end()) {
                continue;
            }

            if (Link.mToNode == Asset::UAnimGraph::OutputNodeId) {
                mRootNode = From->second;

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
            if (Variable.mSource == Asset::EAnimGraphVariableSource::OwnerSpeed) {
                Variable.mValue = Speed;
            }
        }
    }

}
