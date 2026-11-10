#include "Engine/Animation/FAnimNode_Base.h"
#include "Engine/Animation/UAnimInstance.h"

namespace Engine {

    FAnimationUpdateContext::FAnimationUpdateContext(Engine::UAnimInstance& AnimInstance, float DeltaTime, Core::Uint64 UpdateCounter)
        : mAnimInstance{&AnimInstance},
          mDeltaTime{DeltaTime},
          mUpdateCounter{UpdateCounter} {
    }

    Engine::FAnimationUpdateContext FAnimationUpdateContext::FractionalWeight(float Multiplier) const {
        Engine::FAnimationUpdateContext Result{*this};

        Result.mWeight *= Multiplier;

        return Result;
    }

    Engine::UAnimInstance& FAnimationUpdateContext::GetAnimInstance() const {
        return *mAnimInstance;
    }

    float FAnimationUpdateContext::GetDeltaTime() const {
        return mDeltaTime;
    }

    float FAnimationUpdateContext::GetFinalBlendWeight() const {
        return mWeight;
    }

    Core::Uint64 FAnimationUpdateContext::GetUpdateCounter() const {
        return mUpdateCounter;
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
        mLastUpdateCounter = 0;
        mLastWeight = 0.0f;

        if (mLinkedNode != nullptr) {
            mLinkedNode->Initialize(AnimInstance);
        }
    }

    void FPoseLink::Update(const Engine::FAnimationUpdateContext& Context) {
        if (mLinkedNode != nullptr) {
            if (mLastUpdateCounter != Context.GetUpdateCounter()) {
                mLastUpdateCounter = Context.GetUpdateCounter();
                mLastWeight = 0.0f;
            }

            mLastWeight += Context.GetFinalBlendWeight();
            mLinkedNode->RecordUpdate(Context);
            mLinkedNode->Update(Context);
        }
    }

    void FPoseLink::Evaluate(Engine::FPoseContext& Output) {
        if (mLinkedNode != nullptr) {
            mLinkedNode->Evaluate(Output);
        }
    }

    float FPoseLink::GetDebugWeight(Core::Uint64 UpdateCounter) const {
        return mLastUpdateCounter == UpdateCounter ? mLastWeight : 0.0f;
    }

    void FAnimNode_Base::Initialize(Engine::UAnimInstance& AnimInstance) {
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

    void FAnimNode_Base::GetPoseInputs(Core::TArray<Engine::FAnimNodePoseInput>&) {
    }

    void FAnimNode_Base::GetDebugValues(Core::TArray<Engine::FAnimNodeValue>&) {
    }

    float FAnimNode_Base::GetDebugWeight(Core::Uint64 UpdateCounter) const {
        return mLastUpdateCounter == UpdateCounter ? mLastWeight : 0.0f;
    }

    void FAnimNode_Base::RecordUpdate(const Engine::FAnimationUpdateContext& Context) {
        // 같은 노드가 여러 입력에 연결되면 한 갱신 안에서 받은 가중치를 더합니다.
        if (mLastUpdateCounter == Context.GetUpdateCounter()) {
            mLastWeight += Context.GetFinalBlendWeight();
        } else {
            mLastWeight = Context.GetFinalBlendWeight();
            mLastUpdateCounter = Context.GetUpdateCounter();
        }
    }

}
