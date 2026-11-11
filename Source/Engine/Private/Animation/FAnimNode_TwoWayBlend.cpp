#include "Engine/Animation/FAnimNode_TwoWayBlend.h"
#include "Engine/Animation/FAnimationRuntime.h"

#include <algorithm>
#include <cmath>

namespace Engine {

    void FAnimNode_TwoWayBlend::Update(const Engine::FAnimationUpdateContext& Context) {
        mAlpha = mAlphaInput.Evaluate(Context, mAlpha);

        mBlendAlpha = std::isfinite(mAlpha) ? std::clamp(mAlpha, 0.0f, 1.0f) : 0.0f;

        // 각 입력은 자기 가중치만큼 기여하므로 가중치를 나눠 전달하고, 기여하지 않는 입력은 갱신하지 않습니다.
        if (mBlendAlpha < 1.0f - RelevantWeightThreshold) {
            mA.Update(Context);
        }

        if (mBlendAlpha > RelevantWeightThreshold) {
            mB.Update(Context);
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

        const float WeightA{1.0f - mBlendAlpha};
        mA.Evaluate(PoseA, WeightA);
        mB.Evaluate(PoseB, 1.0f - WeightA);

        // 본마다 위치·스케일은 Lerp, 회전은 Slerp로 섞습니다.
        Engine::FAnimationRuntime::BlendTwoPosesTogether(PoseA.mPose, PoseB.mPose, WeightA, Output.mPose);
    }

    void FAnimNode_TwoWayBlend::GetValueInputs(Core::TArray<Engine::FAnimNodeValueInput>& OutInputs) {
        OutInputs.push_back({"Alpha", &mAlphaInput});
    }

    const Core::FString& FAnimNode_TwoWayBlend::GetNodeType() const {
        static const Core::FString Type{"Two-Way Blend"};

        return Type;
    }

    void FAnimNode_TwoWayBlend::GetPoseInputs(Core::TArray<Engine::FAnimNodePoseInput>& OutInputs) {
        OutInputs.push_back({"A", &mA});
        OutInputs.push_back({"B", &mB});
    }

    void FAnimNode_TwoWayBlend::GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues) {
        Engine::FAnimNodeValue Alpha{};

        Alpha.mName = "Alpha";
        Alpha.mValue = Engine::FAnimNodeFloatValue{&mAlpha, 0.0f, 1.0f};
        OutValues.push_back(std::move(Alpha));
    }

    float FAnimNode_TwoWayBlend::GetBlendAlpha() const {
        return mBlendAlpha;
    }

}
