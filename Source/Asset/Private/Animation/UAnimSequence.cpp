#include "Asset/Animation/UAnimSequence.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Asset {

    bool UAnimSequence::SetDataModel(Asset::FAnimDataModel DataModel, Core::FString& Error) {
        if (!DataModel.Validate(Error)) {
            return false;
        }

        mDataModel = std::move(DataModel);
        mRevision = Core::FRevision::Next();

        return true;
    }

    const Asset::FAnimDataModel& UAnimSequence::GetDataModel() const {
        return mDataModel;
    }

    void UAnimSequence::GetTrackPose(double Time, bool Looping, Core::TArray<Core::FTransform>& OutTrackPose) const {
        const auto& Tracks{mDataModel.GetBoneAnimationTracks()};
        const double Length{mDataModel.GetPlayLength()};

        OutTrackPose.resize(Tracks.size());

        if (Tracks.empty()) {
            return;
        }

        // 반복 재생이면 [0, 길이) 범위로 감고, 아니면 [0, 길이]로 제한합니다.
        if (Looping && Length > 0.0) {
            Time = std::fmod(Time, Length);

            if (Time < 0.0) {
                Time += Length;
            }
        } else {
            Time = std::clamp(Time, 0.0, Length);
        }

        // 키 격자에서 앞 키와 보간 비율을 구합니다. 마지막 키는 재생 길이 시점이므로 뒤 키는 마지막 키를 넘지 않습니다.
        const double Frame{Time * mDataModel.GetFrameRate().AsDecimal()};
        const std::size_t LastKey{mDataModel.GetNumberOfFrames()};
        const std::size_t Key{std::min(static_cast<std::size_t>(Frame), LastKey)};
        const std::size_t NextKey{std::min(Key + 1, LastKey)};
        const float Alpha{static_cast<float>(Frame - static_cast<double>(Key))};

        for (std::size_t Index{}; Index < Tracks.size(); ++Index) {
            const Asset::FRawAnimSequenceTrack& Track{Tracks[Index].mInternalTrack};
            Core::FTransform& Out{OutTrackPose[Index]};

            Out.mLocation = Track.mPosKeys.size() == 1 ? Track.mPosKeys[0] : Track.mPosKeys[Key] + (Track.mPosKeys[NextKey] - Track.mPosKeys[Key]) * Alpha;
            Out.mRotation = Track.mRotKeys.size() == 1 ? Track.mRotKeys[0] : Track.mRotKeys[Key].Slerp(Track.mRotKeys[NextKey], Alpha);
            Out.mScale = Track.mScaleKeys.size() == 1 ? Track.mScaleKeys[0] : Track.mScaleKeys[Key] + (Track.mScaleKeys[NextKey] - Track.mScaleKeys[Key]) * Alpha;
        }
    }

    double UAnimSequence::GetPlayLength() const {
        return mDataModel.GetPlayLength();
    }

    bool UAnimSequence::IsValid() const {
        return !mDataModel.GetBoneAnimationTracks().empty();
    }

    Core::FRevision UAnimSequence::GetRevision() const {
        return mRevision;
    }

}
