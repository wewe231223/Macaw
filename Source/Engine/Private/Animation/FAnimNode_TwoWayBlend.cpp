#include "Engine/Animation/FAnimNode_TwoWayBlend.h"
#include "Engine/Animation/FAnimationRuntime.h"

#include <algorithm>
#include <cmath>

namespace Engine {

    void FAnimNode_TwoWayBlend::Update(const Engine::FAnimationUpdateContext& Context) {
        if (mAlphaBinding != nullptr) {
            mAlpha = *mAlphaBinding;
        }

        mBlendAlpha = std::isfinite(mAlpha) ? std::clamp(mAlpha, 0.0f, 1.0f) : 0.0f;

        // 각 입력은 자기 가중치만큼 기여하므로 가중치를 나눠 전달하고, 기여하지 않는 입력은 갱신하지 않습니다.
        if (mBlendAlpha < 1.0f - RelevantWeightThreshold) {
            mA.Update(Context.FractionalWeight(1.0f - mBlendAlpha));
        }

        if (mBlendAlpha > RelevantWeightThreshold) {
            mB.Update(Context.FractionalWeight(mBlendAlpha));
        }
    }

    void FAnimNode_TwoWayBlend::Evaluate(Engine::FPoseContext& Output) {
        if (mBlendAlpha <= RelevantWeightThreshold) {
            mA.Evaluate(Output);

            return;
        }

        if (mBlendAlpha >= 1.0f - RelevantWeightThreshold) {
            mB.Evaluate(Output);

            return;
        }

        Engine::FPoseContext PoseA{Output.GetAnimInstance()};
        Engine::FPoseContext PoseB{Output.GetAnimInstance()};

        mA.Evaluate(PoseA);
        mB.Evaluate(PoseB);

        // 본마다 위치·스케일은 Lerp, 회전은 Slerp로 섞습니다.
        Engine::FAnimationRuntime::BlendTwoPosesTogether(PoseA.mPose, PoseB.mPose, 1.0f - mBlendAlpha, Output.mPose);
    }

    const char* FAnimNode_TwoWayBlend::GetNodeType() const {
        return "Two-Way Blend";
    }

    void FAnimNode_TwoWayBlend::GetPoseInputs(Core::TArray<Engine::FAnimNodePoseInput>& OutInputs) {
        OutInputs.push_back({"A", &mA, 1.0f - mBlendAlpha});
        OutInputs.push_back({"B", &mB, mBlendAlpha});
    }

    void FAnimNode_TwoWayBlend::GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues) {
        Engine::FAnimNodeValue Alpha{};

        Alpha.mName = "Alpha";
        Alpha.mType = Engine::EAnimNodeValueType::Float;
        Alpha.mFloat = &mAlpha;
        Alpha.mMin = 0.0f;
        Alpha.mMax = 1.0f;
        Alpha.mBinding = mAlphaBinding != nullptr ? mAlphaBindingName : Core::FString{};
        Alpha.mBindingParameter = "Alpha";
        OutValues.push_back(std::move(Alpha));
    }

    float FAnimNode_TwoWayBlend::GetBlendAlpha() const {
        return mBlendAlpha;
    }

}
