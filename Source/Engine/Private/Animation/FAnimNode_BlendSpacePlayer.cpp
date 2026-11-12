#include "Engine/Animation/FAnimNode_BlendSpacePlayer.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Engine/Animation/FAnimationRuntime.h"
#include "Engine/Animation/UAnimInstance.h"

#include <algorithm>
#include <cmath>

namespace Engine {

    void FAnimNode_BlendSpacePlayer::Initialize(Engine::UAnimInstance& AnimInstance) {
        mSampleWeights.clear();
        mBlendedLength = 0.0;
        Engine::FAnimNode_AssetPlayerBase::Initialize(AnimInstance);
        mNormalizedTime = mPlayRate < 0.0f ? 1.0 : 0.0;
    }

    void FAnimNode_BlendSpacePlayer::Update(const Engine::FAnimationUpdateContext& Context) {
        SetRootMotionDelta({});
        mX = mXInput.Evaluate(Context, mX);

        if (mBlendSpace == nullptr) {
            mSampleWeights.clear();
            mBlendedLength = 0.0;
            mInternalTime = 0.0;

            return;
        }

        mBlendSpace->GetSamplesToBlend({mX, 0.0f, 0.0f}, mSampleWeights);
        mBlendedLength = mBlendSpace->GetBlendedPlayLength(mSampleWeights);

        // 정규화 위치를 초 단위로 바꿔 공용 진행 규칙을 적용한 뒤 다시 정규화합니다. 합성 길이가 바뀌어도 위상은 유지됩니다.
        mInternalTime = mNormalizedTime * mBlendedLength;
        AdvanceTime(Context.GetDeltaTime(), mBlendedLength);
        mNormalizedTime = mBlendedLength > 0.0 ? std::clamp(mInternalTime / mBlendedLength, 0.0, 1.0) : 0.0;

        if (mBlendedLength <= 0.0 || GetAssetTimeDelta() == 0.0) {
            return;
        }

        const double PreviousPhase{GetPreviousAssetTime() / mBlendedLength};
        const double PhaseDelta{GetAssetTimeDelta() / mBlendedLength};
        const auto& Samples{mBlendSpace->GetSamples()};
        Core::FTransform RootMotion{};
        float Accumulated{};

        for (const Asset::FBlendSampleWeight& Weight : mSampleWeights) {
            if (Weight.mSampleIndex < 0 || static_cast<std::size_t>(Weight.mSampleIndex) >= Samples.size() || Weight.mWeight <= 0.0f) {
                continue;
            }

            const Asset::UAnimSequence* Sequence{Samples[static_cast<std::size_t>(Weight.mSampleIndex)].mAnimation};

            if (Sequence == nullptr) {
                continue;
            }

            const double Length{Sequence->GetPlayLength()};
            const Core::FTransform Delta{Context.GetAnimInstance().ExtractSequenceRootMotion(*Sequence, PreviousPhase * Length, PhaseDelta * Length, mLoop)};

            Accumulated += Weight.mWeight;
            RootMotion = Engine::FAnimationRuntime::BlendTransforms(RootMotion, Delta, 1.0f - Weight.mWeight / Accumulated);
        }

        SetRootMotionDelta(RootMotion);
    }

    void FAnimNode_BlendSpacePlayer::Evaluate(Engine::FPoseContext& Output) {
        if (mBlendSpace == nullptr || mSampleWeights.empty()) {
            return;
        }

        const auto& Samples{mBlendSpace->GetSamples()};
        Engine::UAnimInstance& AnimInstance{Output.GetAnimInstance()};
        Engine::FPoseContext SamplePose{AnimInstance};
        float Accumulated{};
        bool First{true};

        // 샘플 포즈를 차례로 누적합니다. 지금까지의 누적 가중치와 새 샘플 가중치의 비율로 섞으면 전체가 가중 평균이 됩니다.
        for (const Asset::FBlendSampleWeight& Weight : mSampleWeights) {
            // 에디터에서 블렌드 스페이스를 저장하면 샘플 수가 바뀔 수 있으므로 지난 Update의 순서를 검사합니다.
            if (Weight.mSampleIndex < 0 || static_cast<std::size_t>(Weight.mSampleIndex) >= Samples.size()) {
                continue;
            }

            const Asset::UAnimSequence* Animation{Samples[static_cast<std::size_t>(Weight.mSampleIndex)].mAnimation};

            if (Animation == nullptr || Weight.mWeight <= 0.0f) {
                continue;
            }

            const double Time{mNormalizedTime * Animation->GetPlayLength()};
            AnimInstance.RecordDebugSample(*this, Weight.mSampleIndex, Weight.mWeight);

            if (First) {
                AnimInstance.EvaluateSequence(*Animation, Time, mLoop, Output.mPose);
                Accumulated = Weight.mWeight;
                First = false;

                continue;
            }

            SamplePose.ResetToReferencePose();
            AnimInstance.EvaluateSequence(*Animation, Time, mLoop, SamplePose.mPose);
            Accumulated += Weight.mWeight;
            Engine::FAnimationRuntime::BlendTwoPosesTogether(Output.mPose, SamplePose.mPose, 1.0f - Weight.mWeight / Accumulated, Output.mPose);
        }
    }

    void FAnimNode_BlendSpacePlayer::GetValueInputs(Core::TArray<Engine::FAnimNodeValueInput>& OutInputs) {
        OutInputs.push_back({"X", &mXInput});
    }

    const Core::FString& FAnimNode_BlendSpacePlayer::GetNodeType() const {
        static const Core::FString Type{"Blend Space Player"};

        return Type;
    }

    void FAnimNode_BlendSpacePlayer::GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues) {
        Engine::FAnimNodeValue BlendSpace{};

        BlendSpace.mName = "Blend Space";
        BlendSpace.mValue = mBlendSpace != nullptr ? mBlendSpace->GetName() : "(없음)";
        OutValues.push_back(std::move(BlendSpace));

        Engine::FAnimNodeValue X{};
        const Asset::FBlendParameter* Parameter{mBlendSpace != nullptr ? &mBlendSpace->GetBlendParameter(Asset::EBlendSpaceAxis::X) : nullptr};

        X.mName = "X";
        X.mValue = Engine::FAnimNodeFloatValue{&mX, Parameter != nullptr ? Parameter->mMin : 0.0f, Parameter != nullptr ? Parameter->mMax : 0.0f};
        OutValues.push_back(std::move(X));

        GetPlayerDebugValues(OutValues);
    }

    double FAnimNode_BlendSpacePlayer::GetCurrentAssetLength() const {
        return mBlendedLength;
    }

    const Core::TArray<Asset::FBlendSampleWeight>& FAnimNode_BlendSpacePlayer::GetSampleWeights() const {
        return mSampleWeights;
    }

    double FAnimNode_BlendSpacePlayer::GetNormalizedTime() const {
        return mNormalizedTime;
    }

}
