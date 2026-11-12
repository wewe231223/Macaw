#include "Engine/Animation/FAnimNode_SequencePlayer.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Engine/Animation/UAnimInstance.h"

namespace Engine {

    void FAnimNode_SequencePlayer::Update(const Engine::FAnimationUpdateContext& Context) {
        SetRootMotionDelta({});

        if (!mPaused) {
            AdvanceTime(Context.GetDeltaTime(), GetCurrentAssetLength());

            if (mSequence != nullptr) {
                SetRootMotionDelta(Context.GetAnimInstance().ExtractSequenceRootMotion(*mSequence, GetPreviousAssetTime(), GetAssetTimeDelta(), mLoop));
            }
        }
    }

    void FAnimNode_SequencePlayer::Evaluate(Engine::FPoseContext& Output) {
        if (mSequence != nullptr) {
            Output.GetAnimInstance().EvaluateSequence(*mSequence, mInternalTime, mLoop, Output.mPose);
        }
    }

    const Core::FString& FAnimNode_SequencePlayer::GetNodeType() const {
        static const Core::FString Type{"Sequence Player"};

        return Type;
    }

    void FAnimNode_SequencePlayer::GetDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues) {
        Engine::FAnimNodeValue Sequence{};

        Sequence.mName = "Sequence";
        Sequence.mValue = mSequence != nullptr ? mSequence->GetName() : "(없음)";
        OutValues.push_back(std::move(Sequence));

        GetPlayerDebugValues(OutValues);
    }

    double FAnimNode_SequencePlayer::GetCurrentAssetLength() const {
        return mSequence != nullptr ? mSequence->GetPlayLength() : 0.0;
    }

    void FAnimNode_SequencePlayer::SetPaused(bool Paused) {
        mPaused = Paused;
    }

}
