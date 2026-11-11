#include "Engine/Animation/FAnimValueNode.h"
#include "Engine/Animation/FAnimNode_Base.h"
#include "Engine/Animation/UAnimInstance.h"

namespace Engine {

    FAnimValueNode_Base::FAnimValueNode_Base(Engine::EAnimValueType ValueType)
        : mValueType{ValueType} {
    }

    Engine::EAnimValueType FAnimValueNode_Base::GetValueType() const {
        return mValueType;
    }

    const Core::FString& FAnimValueNode_Base::GetDisplayName() const {
        return GetNodeType();
    }

    FAnimValueNode_Float::FAnimValueNode_Float()
        : Engine::FAnimValueNode_Base{Engine::EAnimValueType::Float} {
    }

    float FAnimValueNode_GetFloat::Evaluate(const Engine::FAnimationUpdateContext& Context) const {
        return Context.GetAnimInstance().GetFloatVariable(mVariableName);
    }

    const Core::FString& FAnimValueNode_GetFloat::GetNodeType() const {
        static const Core::FString Type{"Get Float"};
        return Type;
    }

    const Core::FString& FAnimValueNode_GetFloat::GetDisplayName() const {
        return mVariableName;
    }

    FAnimValueLink_Base::FAnimValueLink_Base(Engine::EAnimValueType ValueType)
        : mValueType{ValueType} {
    }

    Engine::EAnimValueType FAnimValueLink_Base::GetValueType() const {
        return mValueType;
    }

    FAnimFloatLink::FAnimFloatLink()
        : Engine::FAnimValueLink_Base{Engine::EAnimValueType::Float} {
    }

    Engine::FAnimValueNode_Float* FAnimFloatLink::GetLinkedNode() const {
        return mLinkedNode;
    }

    bool FAnimFloatLink::SetLinkedNode(Engine::FAnimValueNode_Base* Node) {
        if (Node != nullptr && Node->GetValueType() != GetValueType()) {
            return false;
        }

        mLinkedNode = static_cast<Engine::FAnimValueNode_Float*>(Node);
        return true;
    }

    float FAnimFloatLink::Evaluate(const Engine::FAnimationUpdateContext& Context, float DefaultValue) const {
        return mLinkedNode != nullptr ? mLinkedNode->Evaluate(Context) : DefaultValue;
    }

}
