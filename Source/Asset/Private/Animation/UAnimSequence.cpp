/**
 * @file UAnimSequence.cpp
 * @brief 임포트한 본 트랙과 프레임 레이트를 소유하는 애니메이션 클립입니다.
 * @ingroup Asset
 */

#include "Asset/Animation/UAnimSequence.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Asset {

    /**
     * @brief 임포트한 본 트랙과 프레임 레이트를 소유하는 애니메이션 클립의 파일 내부 보조 구현입니다.
     */
    namespace {

        /**
         * @brief 두 키 사이의 위치·회전·크기를 보간합니다.
         * @param Track 보간할 원시 애니메이션 트랙입니다.
         * @param Key 보간 구간의 시작 키 인덱스입니다.
         * @param NextKey 보간 구간의 다음 키 인덱스입니다.
         * @param Alpha 0~1의 보간 가중치입니다.
         * @return 보간한 트랙 로컬 변환입니다.
         */
        Core::FTransform SampleAnimationTrack(const Asset::FRawAnimSequenceTrack& Track, std::size_t Key, std::size_t NextKey, float Alpha) {
            return {Track.mPosKeys.size() == 1 ? Track.mPosKeys[0] : Track.mPosKeys[Key] + (Track.mPosKeys[NextKey] - Track.mPosKeys[Key]) * Alpha, Track.mRotKeys.size() == 1 ? Track.mRotKeys[0] : Track.mRotKeys[Key].Slerp(Track.mRotKeys[NextKey], Alpha), Track.mScaleKeys.size() == 1 ? Track.mScaleKeys[0] : Track.mScaleKeys[Key] + (Track.mScaleKeys[NextKey] - Track.mScaleKeys[Key]) * Alpha};
        }

        /**
         * @brief 연속한 두 루트 모션 변환을 적용 순서에 맞게 합성합니다.
         * @param A 먼저 적용하거나 비교할 변환입니다.
         * @param B 다음으로 적용하거나 비교할 변환입니다.
         * @return A 다음 B를 적용하는 루트 모션 변환입니다.
         */
        Core::FTransform ComposeRootMotion(const Core::FTransform& A, const Core::FTransform& B) {
            return {A.mLocation + A.mRotation.RotateVector(B.mLocation), (A.mRotation * B.mRotation).Normalized(), {1.0f, 1.0f, 1.0f}};
        }
    }

    bool UAnimSequence::SetDataModel(Asset::FAnimDataModel DataModel, Core::FString& Error) {
        if (!DataModel.Validate(Error)) {
            return false;
        }

        mDataModel = std::move(DataModel);

        return true;
    }

    const Asset::FAnimDataModel& UAnimSequence::GetDataModel() const {
        return mDataModel;
    }

    const Core::FString& UAnimSequence::GetAssetPath() const {
        return mAssetPath;
    }

    bool UAnimSequence::IsRootMotionEnabled() const {
        return mEnableRootMotion;
    }

    void UAnimSequence::SetRootMotionEnabled(bool Enabled) {
        mEnableRootMotion = Enabled;
    }

    Asset::ERootMotionRootLock UAnimSequence::GetRootMotionRootLock() const {
        return mRootMotionRootLock;
    }

    void UAnimSequence::SetRootMotionRootLock(Asset::ERootMotionRootLock RootLock) {
        if (RootLock >= Asset::ERootMotionRootLock::RefPose && RootLock <= Asset::ERootMotionRootLock::Zero) {
            mRootMotionRootLock = RootLock;
        }
    }

    bool UAnimSequence::IsRootLockForced() const {
        return mForceRootLock;
    }

    void UAnimSequence::SetForceRootLock(bool Forced) {
        mForceRootLock = Forced;
    }

    const Core::FString& UAnimSequence::GetRootMotionBone() const {
        return mRootMotionBone;
    }

    bool UAnimSequence::SetRootMotionBone(const Core::FString& BoneName) {
        if (BoneName.find('\0') != Core::FString::npos) {
            return false;
        }

        mRootMotionBone = BoneName;

        return true;
    }

    Core::FTransform UAnimSequence::SampleTrackTransform(std::size_t TrackIndex, double Time) const {
        const auto& Tracks{mDataModel.GetBoneAnimationTracks()};

        if (TrackIndex >= Tracks.size()) {
            return {};
        }

        Time = std::clamp(std::isfinite(Time) ? Time : 0.0, 0.0, GetPlayLength());

        const double Frame{Time * mDataModel.GetFrameRate().AsDecimal()};
        const std::size_t LastKey{mDataModel.GetNumberOfFrames()};
        const std::size_t Key{std::min(static_cast<std::size_t>(Frame), LastKey)};
        const std::size_t NextKey{std::min(Key + 1, LastKey)};
        const float Alpha{static_cast<float>(std::clamp(Frame - static_cast<double>(Key), 0.0, 1.0))};

        return SampleAnimationTrack(Tracks[TrackIndex].mInternalTrack, Key, NextKey, Alpha);
    }

    Core::FTransform UAnimSequence::GetRootTrackTransform(Core::Int32 TrackIndex, double Time) const {
        return TrackIndex >= 0 ? SampleTrackTransform(static_cast<std::size_t>(TrackIndex), Time) : Core::FTransform{};
    }

    Core::FTransform UAnimSequence::GetRootLockTransform(Core::Int32 TrackIndex, const Core::FTransform& ReferencePose) const {
        switch (mRootMotionRootLock) {
        case Asset::ERootMotionRootLock::RefPose:
            return ReferencePose;
        case Asset::ERootMotionRootLock::AnimFirstFrame:
            return GetRootTrackTransform(TrackIndex, 0.0);
        case Asset::ERootMotionRootLock::Zero:
            return {};
        }

        return ReferencePose;
    }

    Core::FTransform UAnimSequence::ExtractRootMotionFromRange(Core::Int32 TrackIndex, double StartTime, double EndTime) const {
        if (TrackIndex < 0 || static_cast<std::size_t>(TrackIndex) >= mDataModel.GetBoneAnimationTracks().size() || !std::isfinite(StartTime) || !std::isfinite(EndTime)) {
            return {};
        }

        const Core::FTransform Start{GetRootTrackTransform(TrackIndex, StartTime)};
        const Core::FTransform End{GetRootTrackTransform(TrackIndex, EndTime)};
        const Core::FQuat Inverse{Start.mRotation.Inverse()};

        return {Inverse.RotateVector(End.mLocation - Start.mLocation), (Inverse * End.mRotation).Normalized(), {1.0f, 1.0f, 1.0f}};
    }

    Core::FTransform UAnimSequence::ExtractRootMotion(Core::Int32 TrackIndex, double StartTime, double DeltaTime, bool Looping) const {
        const double Length{GetPlayLength()};

        if (Length <= 0.0 || !std::isfinite(StartTime) || !std::isfinite(DeltaTime) || DeltaTime == 0.0) {
            return {};
        }

        if (!Looping) {
            return ExtractRootMotionFromRange(TrackIndex, StartTime, StartTime + DeltaTime);
        }

        StartTime = std::fmod(StartTime, Length);

        if (StartTime < 0.0) {
            StartTime += Length;
        }

        const bool Forward{DeltaTime > 0.0};
        const double FirstDistance{Forward ? Length - StartTime : StartTime};
        double Remaining{std::abs(DeltaTime)};

        if (Remaining <= FirstDistance) {
            return ExtractRootMotionFromRange(TrackIndex, StartTime, StartTime + DeltaTime);
        }

        Core::FTransform Result{ExtractRootMotionFromRange(TrackIndex, StartTime, Forward ? Length : 0.0)};
        Remaining -= FirstDistance;

        double Cycles{std::floor(Remaining / Length)};

        if (!std::isfinite(Cycles)) {
            return {};
        }

        Core::FTransform Cycle{ExtractRootMotionFromRange(TrackIndex, Forward ? 0.0 : Length, Forward ? Length : 0.0)};

        while (Cycles >= 1.0) {
            if (std::fmod(Cycles, 2.0) >= 1.0) {
                Result = ComposeRootMotion(Result, Cycle);
            }

            Cycle = ComposeRootMotion(Cycle, Cycle);
            Cycles = std::floor(Cycles * 0.5);
        }

        const double Tail{std::fmod(Remaining, Length)};

        return ComposeRootMotion(Result, ExtractRootMotionFromRange(TrackIndex, Forward ? 0.0 : Length, Forward ? Tail : Length - Tail));
    }

    void UAnimSequence::GetTrackPose(double Time, bool Looping, Core::TArray<Core::FTransform>& OutTrackPose) const {
        const auto& Tracks{mDataModel.GetBoneAnimationTracks()};
        const double Length{mDataModel.GetPlayLength()};

        OutTrackPose.resize(Tracks.size());

        if (Tracks.empty()) {
            return;
        }

        Time = std::isfinite(Time) ? Time : 0.0;

        // 반복 재생이면 [0, 길이) 범위로 감고, 아니면 [0, 길이]로 제한합니다.
        if (Looping && Length > 0.0) {
            Time = std::fmod(Time, Length);

            if (Time < 0.0) {
                Time += Length;
            }
        } else {
            Time = std::clamp(Time, 0.0, Length);
        }

        const double Frame{Time * mDataModel.GetFrameRate().AsDecimal()};
        const std::size_t LastKey{mDataModel.GetNumberOfFrames()};
        const std::size_t Key{std::min(static_cast<std::size_t>(Frame), LastKey)};
        const std::size_t NextKey{std::min(Key + 1, LastKey)};
        const float Alpha{static_cast<float>(std::clamp(Frame - static_cast<double>(Key), 0.0, 1.0))};

        for (std::size_t Index{}; Index < Tracks.size(); ++Index) {
            OutTrackPose[Index] = SampleAnimationTrack(Tracks[Index].mInternalTrack, Key, NextKey, Alpha);
        }
    }

    double UAnimSequence::GetPlayLength() const {
        return mDataModel.GetPlayLength();
    }

    bool UAnimSequence::IsValid() const {
        return !mDataModel.GetBoneAnimationTracks().empty();
    }

}
