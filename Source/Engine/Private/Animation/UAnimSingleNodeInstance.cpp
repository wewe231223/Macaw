#include "Engine/Animation/UAnimSingleNodeInstance.h"
#include "Asset/Animation/UAnimSequence.h"

namespace Engine {

    UAnimSingleNodeInstance::UAnimSingleNodeInstance() = default;

    UAnimSingleNodeInstance::~UAnimSingleNodeInstance() = default;

    void UAnimSingleNodeInstance::SetAnimation(const Asset::UAnimSequence* Sequence) {
        ResetRootMotion();
        mPlayer.mSequence = Sequence;
        mPlayer.Restart();
    }

    const Asset::UAnimSequence* UAnimSingleNodeInstance::GetAnimation() const {
        return mPlayer.mSequence;
    }

    void UAnimSingleNodeInstance::Play(bool Looping) {
        mPlayer.mLoop = Looping;
        mPlaying = true;

        // 반복하지 않는 재생이 끝난 뒤 다시 재생하면 처음부터 시작합니다.
        if (mPlayer.IsFinished()) {
            mPlayer.Restart();
        }
    }

    void UAnimSingleNodeInstance::Stop() {
        ResetRootMotion();
        mPlaying = false;
    }

    bool UAnimSingleNodeInstance::IsPlaying() const {
        return mPlaying;
    }

    void UAnimSingleNodeInstance::SetLooping(bool Looping) {
        mPlayer.mLoop = Looping;
    }

    void UAnimSingleNodeInstance::SetPlayRate(float PlayRate) {
        mPlayer.mPlayRate = PlayRate;
    }

    void UAnimSingleNodeInstance::SetPosition(double Time) {
        ResetRootMotion();
        mPlayer.SetAccumulatedTime(Time);
    }

    double UAnimSingleNodeInstance::GetPosition() const {
        return mPlayer.GetAccumulatedTime();
    }

    void UAnimSingleNodeInstance::NativeInitializeAnimation() {
        mPlayer.mName = "Single Node";
        mRootNode = &mPlayer;
    }

    void UAnimSingleNodeInstance::NativeUpdateAnimation(float) {
        // 반복하지 않는 재생은 플레이어가 끝에서 멈추면 재생 상태도 끝냅니다.
        if (mPlayer.IsFinished()) {
            mPlaying = false;
        }

        mPlayer.SetPaused(!mPlaying);
    }

}
