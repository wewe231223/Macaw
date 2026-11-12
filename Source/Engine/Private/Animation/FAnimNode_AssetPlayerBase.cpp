#include "Engine/Animation/FAnimNode_AssetPlayerBase.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace Engine {

    void FAnimNode_AssetPlayerBase::Initialize(Engine::UAnimInstance& AnimInstance) {
        Engine::FAnimNode_Base::Initialize(AnimInstance);
        Restart();
    }

    double FAnimNode_AssetPlayerBase::GetAccumulatedTime() const {
        return mInternalTime;
    }

    void FAnimNode_AssetPlayerBase::SetAccumulatedTime(double Time) {
        mInternalTime = std::clamp(std::isfinite(Time) ? Time : 0.0, 0.0, std::max(GetCurrentAssetLength(), 0.0));
        mFinished = false;
        mPreviousTime = mInternalTime;
        mAssetTimeDelta = 0.0;
        mRootMotionDelta = {};
    }

    bool FAnimNode_AssetPlayerBase::IsFinished() const {
        return mFinished;
    }

    void FAnimNode_AssetPlayerBase::Restart() {
        SetAccumulatedTime(mPlayRate < 0.0f ? std::max(GetCurrentAssetLength(), 0.0) : 0.0);
    }

    double FAnimNode_AssetPlayerBase::GetPreviousAssetTime() const {
        return mPreviousTime;
    }

    double FAnimNode_AssetPlayerBase::GetAssetTimeDelta() const {
        return mAssetTimeDelta;
    }

    Core::FTransform FAnimNode_AssetPlayerBase::GetRootMotionDelta() const {
        return mRootMotionDelta;
    }

    void FAnimNode_AssetPlayerBase::SetRootMotionDelta(const Core::FTransform& Delta) {
        mRootMotionDelta = Delta;
    }

    void FAnimNode_AssetPlayerBase::AdvanceTime(float DeltaTime, double Length) {
        mPreviousTime = mInternalTime;
        mAssetTimeDelta = 0.0;

        if (Length <= 0.0 || !std::isfinite(Length) || !std::isfinite(mPlayRate)) {
            mInternalTime = 0.0;

            return;
        }

        if (!std::isfinite(DeltaTime) || DeltaTime <= 0.0f || mPlayRate == 0.0f) {
            return;
        }

        const double Advance{static_cast<double>(DeltaTime) * static_cast<double>(mPlayRate)};
        const double Next{mInternalTime + Advance};

        mInternalTime = Next;

        // 반복 재생이면 [0, 길이)로 감아 오차가 쌓이지 않게 하고, 아니면 재생 방향의 끝에서 멈춥니다.
        if (mLoop) {
            mInternalTime = std::fmod(mInternalTime, Length);

            if (mInternalTime < 0.0) {
                mInternalTime += Length;
            }

            mFinished = false;
            mAssetTimeDelta = Advance;
        } else if (mInternalTime >= Length || mInternalTime <= 0.0) {
            mFinished = (mPlayRate > 0.0f && mInternalTime >= Length) || (mPlayRate < 0.0f && mInternalTime <= 0.0);
            mInternalTime = std::clamp(mInternalTime, 0.0, Length);
        }

        if (!mLoop) {
            mAssetTimeDelta = mInternalTime - mPreviousTime;
        }
    }

    void FAnimNode_AssetPlayerBase::GetPlayerDebugValues(Core::TArray<Engine::FAnimNodeValue>& OutValues) {
        const double Length{GetCurrentAssetLength()};
        Engine::FAnimNodeValue Time{};

        Time.mName = "Time";
        Time.mValue = std::format("{:.2f} / {:.2f} s{}", mInternalTime, Length, mFinished ? " (끝)" : "");
        OutValues.push_back(std::move(Time));

        Engine::FAnimNodeValue PlayRate{};

        PlayRate.mName = "Play Rate";
        PlayRate.mValue = Engine::FAnimNodeFloatValue{&mPlayRate, -3.0f, 3.0f};
        OutValues.push_back(std::move(PlayRate));

        Engine::FAnimNodeValue Loop{};

        Loop.mName = "Loop";
        Loop.mValue = &mLoop;
        OutValues.push_back(std::move(Loop));
    }

}
