#include "Engine/Animation/UAnimSingleNodeInstance.h"
#include "Asset/Animation/UAnimSequence.h"

#include <cmath>

namespace Engine {

    UAnimSingleNodeInstance::UAnimSingleNodeInstance() = default;

    UAnimSingleNodeInstance::~UAnimSingleNodeInstance() = default;

    void UAnimSingleNodeInstance::SetAnimation(const Asset::UAnimSequence* Sequence) {
        mSequence = Sequence;
        mPosition = 0.0;
        ResetTrackMapping();
    }

    const Asset::UAnimSequence* UAnimSingleNodeInstance::GetAnimation() const {
        return mSequence;
    }

    void UAnimSingleNodeInstance::Play(bool Looping) {
        mLooping = Looping;
        mPlaying = true;
    }

    void UAnimSingleNodeInstance::Stop() {
        mPlaying = false;
    }

    bool UAnimSingleNodeInstance::IsPlaying() const {
        return mPlaying;
    }

    void UAnimSingleNodeInstance::SetLooping(bool Looping) {
        mLooping = Looping;
    }

    void UAnimSingleNodeInstance::SetPlayRate(float PlayRate) {
        mPlayRate = PlayRate;
    }

    void UAnimSingleNodeInstance::SetPosition(double Time) {
        mPosition = Time;
    }

    double UAnimSingleNodeInstance::GetPosition() const {
        return mPosition;
    }

    void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds) {
        if (!mPlaying || mSequence == nullptr) {
            return;
        }

        const double Length{mSequence->GetPlayLength()};

        mPosition += static_cast<double>(DeltaSeconds) * static_cast<double>(mPlayRate);

        // 반복 재생이면 위치를 [0, 길이)로 감아 오차가 쌓이지 않게 하고, 아니면 끝에서 멈춥니다.
        if (mLooping && Length > 0.0) {
            mPosition = std::fmod(mPosition, Length);

            if (mPosition < 0.0) {
                mPosition += Length;
            }
        } else if ((mPlayRate > 0.0f && mPosition >= Length) || (mPlayRate < 0.0f && mPosition <= 0.0)) {
            mPosition = mPlayRate > 0.0f ? Length : 0.0;
            mPlaying = false;
        }
    }

    void UAnimSingleNodeInstance::NativeEvaluateAnimation(Core::TArray<Core::FTransform>& Pose) {
        if (mSequence != nullptr) {
            EvaluateSequence(*mSequence, mPosition, mLooping, Pose);
        }
    }

}
