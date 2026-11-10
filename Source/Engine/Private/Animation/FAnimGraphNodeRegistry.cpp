#include "Engine/Animation/FAnimGraphNodeRegistry.h"
#include "Asset/Animation/UAnimGraph.h"
#include "Engine/Animation/FAnimNode_BlendSpacePlayer.h"
#include "Engine/Animation/FAnimNode_SequencePlayer.h"
#include "Engine/Animation/FAnimNode_TwoWayBlend.h"

#include <algorithm>
#include <charconv>
#include <cmath>

namespace Engine {

    namespace {
        const Core::FString* FindDefault(const Core::TArray<Engine::FAnimGraphParameterInfo>* Parameters, const Core::FString& Name) {
            if (Parameters == nullptr) {
                return nullptr;
            }

            const auto Found{std::find_if(Parameters->begin(), Parameters->end(), [&Name](const Engine::FAnimGraphParameterInfo& Info) {
                return Info.mName == Name;
            })};

            return Found != Parameters->end() ? &Found->mDefault : nullptr;
        }

        Core::TArray<Engine::FAnimGraphNodeType> CreateNodeTypes() {
            Core::TArray<Engine::FAnimGraphNodeType> Types{};

            const Engine::FAnimGraphParameterInfo PlayRate{"PlayRate", Engine::EAnimGraphParameterType::Float, "1", false, -3.0f, 3.0f};
            const Engine::FAnimGraphParameterInfo Loop{"Loop", Engine::EAnimGraphParameterType::Bool, "true"};

            Types.push_back({"SequencePlayer", "Sequence Player", {}, {{"Sequence", Engine::EAnimGraphParameterType::AnimSequence}, PlayRate, Loop}, [](const Engine::FAnimGraphNodeBuildContext& Context) {
                auto Node{std::make_unique<Engine::FAnimNode_SequencePlayer>()};

                Node->mSequence = Context.mLoadSequence ? Context.mLoadSequence(Context.GetString("Sequence")) : nullptr;
                Node->mPlayRate = Context.GetFloat("PlayRate");
                Node->mLoop = Context.GetBool("Loop");

                return std::unique_ptr<Engine::FAnimNode_Base>{std::move(Node)};
            }});

            Types.push_back({"TwoWayBlend", "Two-Way Blend", {"A", "B"}, {{"Alpha", Engine::EAnimGraphParameterType::Float, "0", true, 0.0f, 1.0f}}, [](const Engine::FAnimGraphNodeBuildContext& Context) {
                auto Node{std::make_unique<Engine::FAnimNode_TwoWayBlend>()};

                Node->mAlpha = Context.GetFloat("Alpha");
                Node->mAlphaBinding = Context.GetBinding("Alpha");
                Node->mAlphaBindingName = Context.GetBindingName("Alpha");

                return std::unique_ptr<Engine::FAnimNode_Base>{std::move(Node)};
            }});

            Types.push_back({"BlendSpacePlayer", "Blend Space Player", {}, {{"BlendSpace", Engine::EAnimGraphParameterType::BlendSpace}, {"X", Engine::EAnimGraphParameterType::Float, "0", true}, PlayRate, Loop}, [](const Engine::FAnimGraphNodeBuildContext& Context) {
                auto Node{std::make_unique<Engine::FAnimNode_BlendSpacePlayer>()};

                Node->mBlendSpace = Context.mLoadBlendSpace ? Context.mLoadBlendSpace(Context.GetString("BlendSpace")) : nullptr;
                Node->mX = Context.GetFloat("X");
                Node->mXBinding = Context.GetBinding("X");
                Node->mXBindingName = Context.GetBindingName("X");
                Node->mPlayRate = Context.GetFloat("PlayRate");
                Node->mLoop = Context.GetBool("Loop");

                return std::unique_ptr<Engine::FAnimNode_Base>{std::move(Node)};
            }});

            return Types;
        }
    }

    float FAnimGraphNodeBuildContext::GetFloat(const Core::FString& Name) const {
        const Core::FString Text{GetString(Name)};
        float Value{};
        const auto [End, Code]{std::from_chars(Text.data(), Text.data() + Text.size(), Value)};

        if (Code == std::errc{} && End == Text.data() + Text.size() && std::isfinite(Value)) {
            return Value;
        }

        // 저장된 값을 해석할 수 없으면 기본값을 다시 해석합니다.
        const Core::FString* Default{FindDefault(mParameters, Name)};

        if (Default != nullptr && std::from_chars(Default->data(), Default->data() + Default->size(), Value).ec == std::errc{}) {
            return Value;
        }

        return 0.0f;
    }

    bool FAnimGraphNodeBuildContext::GetBool(const Core::FString& Name) const {
        return GetString(Name) == "true";
    }

    Core::FString FAnimGraphNodeBuildContext::GetString(const Core::FString& Name) const {
        if (mNode != nullptr) {
            if (const Core::FString* Value{mNode->FindParameter(Name)}) {
                return *Value;
            }
        }

        const Core::FString* Default{FindDefault(mParameters, Name)};

        return Default != nullptr ? *Default : Core::FString{};
    }

    Core::FString FAnimGraphNodeBuildContext::GetBindingName(const Core::FString& Name) const {
        const Core::FString* Value{mNode != nullptr ? mNode->FindParameter(Name + ".Binding") : nullptr};

        return Value != nullptr ? *Value : Core::FString{};
    }

    const float* FAnimGraphNodeBuildContext::GetBinding(const Core::FString& Name) const {
        const Core::FString Variable{GetBindingName(Name)};

        return !Variable.empty() && mFindVariable ? mFindVariable(Variable) : nullptr;
    }

    const Core::TArray<Engine::FAnimGraphNodeType>& FAnimGraphNodeRegistry::GetNodeTypes() {
        static const Core::TArray<Engine::FAnimGraphNodeType> Types{CreateNodeTypes()};

        return Types;
    }

    const Engine::FAnimGraphNodeType* FAnimGraphNodeRegistry::Find(const Core::FString& Type) {
        const auto& Types{GetNodeTypes()};
        const auto Found{std::find_if(Types.begin(), Types.end(), [&Type](const Engine::FAnimGraphNodeType& Entry) {
            return Entry.mType == Type;
        })};

        return Found != Types.end() ? &*Found : nullptr;
    }

}
