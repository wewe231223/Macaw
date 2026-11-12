#include "Engine/Animation/FAnimNode_Base.h"
#include "Engine/Animation/UAnimInstance.h"

namespace Engine {

    FAnimationUpdateContext::FAnimationUpdateContext(Engine::UAnimInstance& AnimInstance, float DeltaTime)
        : mAnimInstance{&AnimInstance},
          mDeltaTime{DeltaTime} {
    }

    Engine::UAnimInstance& FAnimationUpdateContext::GetAnimInstance() const {
        return *mAnimInstance;
    }

    float FAnimationUpdateContext::GetDeltaTime() const {
        return mDeltaTime;
    }

    FPoseContext::FPoseContext(Engine::UAnimInstance& AnimInstance)
        : mAnimInstance{&AnimInstance} {
        ResetToReferencePose();
    }

    Engine::UAnimInstance& FPoseContext::GetAnimInstance() const {
        return *mAnimInstance;
    }

    void FPoseContext::ResetToReferencePose() {
        mAnimInstance->GetReferencePose(mPose);
    }

    void FPoseLink::Initialize(Engine::UAnimInstance& AnimInstance) {
        if (mLinkedNode != nullptr) {
            mLinkedNode->Initialize(AnimInstance);
        }
    }

    void FPoseLink::Update(const Engine::FAnimationUpdateContext& Context) {
        if (mLinkedNode != nullptr) {
            Context.GetAnimInstance().UpdateNode(*mLinkedNode, Context);
        }
    }

    void FPoseLink::Evaluate(Engine::FPoseContext& Output, float Weight) {
        if (mLinkedNode != nullptr) {
            Engine::UAnimInstance& AnimInstance{Output.GetAnimInstance()};
            AnimInstance.BeginDebugVisit(*mLinkedNode, this, Weight);
            mLinkedNode->Evaluate(Output);
            AnimInstance.EndDebugVisit();
        }
    }

    void FAnimNode_Base::Initialize(Engine::UAnimInstance& AnimInstance) {
        AnimInstance.ResetNodeUpdate(*this);
        Core::TArray<Engine::FAnimNodePoseInput> Inputs{};

        GetPoseInputs(Inputs);

        for (const Engine::FAnimNodePoseInput& Input : Inputs) {
            Input.mLink->Initialize(AnimInstance);
        }
    }

    void FAnimNode_Base::Update(const Engine::FAnimationUpdateContext&) {
    }

    void FAnimNode_Base::Evaluate(Engine::FPoseContext&) {
    }

    Core::FTransform FAnimNode_Base::GetRootMotionDelta() const {
        return {};
    }

    void FAnimNode_Base::GetPoseInputs(Core::TArray<Engine::FAnimNodePoseInput>&) {
    }

    void FAnimNode_Base::GetDebugValues(Core::TArray<Engine::FAnimNodeValue>&) {
    }

    void FAnimNode_Base::GetValueInputs(Core::TArray<Engine::FAnimNodeValueInput>&) {
    }

}
