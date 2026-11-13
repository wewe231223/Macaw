/**
 * @file FAnimDataModel.cpp
 * @brief 시퀀스의 원시 트랙·키프레임과 프레임률 데이터를 관리합니다.
 * @ingroup Asset
 */

#include "Asset/Animation/FAnimDataModel.h"

#include <unordered_set>

namespace Asset {

    bool Asset::FAnimDataModel::Validate(Core::FString& Error) const {
        if (!mFrameRate.IsValid()) {
            Error = "The animation frame rate is invalid";

            return false;
        }

        if (mNumberOfFrames == 0 || mNumberOfFrames > MaxFrames) {
            Error = "The animation frame count is out of range";

            return false;
        }

        if (mBoneAnimationTracks.empty() || mBoneAnimationTracks.size() > MaxTracks) {
            Error = "The animation track count is out of range";

            return false;
        }

        const std::size_t Keys{GetNumberOfKeys()};
        std::unordered_set<Core::FString> Names{};

        Names.reserve(mBoneAnimationTracks.size());

        for (const Asset::FBoneAnimationTrack& Track : mBoneAnimationTracks) {
            const Asset::FRawAnimSequenceTrack& Raw{Track.mInternalTrack};
            const auto IsKeyCount{[Keys](std::size_t Count) {
                return Count == 1 || Count == Keys;
            }};

            if (Track.mName.empty() || Track.mName.find('\0') != Core::FString::npos || !Names.insert(Track.mName).second) {
                Error = "The animation has an empty or duplicate bone name: " + Track.mName;

                return false;
            }

            if (!IsKeyCount(Raw.mPosKeys.size()) || !IsKeyCount(Raw.mRotKeys.size()) || !IsKeyCount(Raw.mScaleKeys.size())) {
                Error = "The animation track key count does not match the frame count: " + Track.mName;

                return false;
            }

            for (const Core::FVector& Key : Raw.mPosKeys) {
                if (!Key.IsFinite()) {
                    Error = "The animation track has a non-finite position: " + Track.mName;

                    return false;
                }
            }

            for (const Core::FQuat& Key : Raw.mRotKeys) {
                if (!Key.IsFinite() || !Key.IsNormalized(0.001f)) {
                    Error = "The animation track has an invalid rotation: " + Track.mName;

                    return false;
                }
            }

            for (const Core::FVector& Key : Raw.mScaleKeys) {
                if (!Key.IsFinite()) {
                    Error = "The animation track has a non-finite scale: " + Track.mName;

                    return false;
                }
            }
        }

        return true;
    }

    const Asset::FFrameRate& Asset::FAnimDataModel::GetFrameRate() const {
        return mFrameRate;
    }

    void Asset::FAnimDataModel::SetFrameRate(const Asset::FFrameRate& FrameRate) {
        mFrameRate = FrameRate;
    }

    Core::Uint32 Asset::FAnimDataModel::GetNumberOfFrames() const {
        return mNumberOfFrames;
    }

    void Asset::FAnimDataModel::SetNumberOfFrames(Core::Uint32 NumberOfFrames) {
        mNumberOfFrames = NumberOfFrames;
    }

    Core::Uint32 Asset::FAnimDataModel::GetNumberOfKeys() const {
        return mNumberOfFrames + 1;
    }

    double Asset::FAnimDataModel::GetPlayLength() const {
        return mFrameRate.AsSeconds(static_cast<double>(mNumberOfFrames));
    }

    const Core::TArray<Asset::FBoneAnimationTrack>& Asset::FAnimDataModel::GetBoneAnimationTracks() const {
        return mBoneAnimationTracks;
    }

    Core::TArray<Asset::FBoneAnimationTrack>& Asset::FAnimDataModel::GetBoneAnimationTracks() {
        return mBoneAnimationTracks;
    }

}
