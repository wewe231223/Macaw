/**
 * @file FSequenceTransformSection.cpp
 * @brief 시간 구간의 위치·회전·스케일 채널에 대한 선언입니다.
 * @ingroup Asset
 */

#include "Core/Types.h"
#include "Asset/Sequence/FSequenceTransformSection.h"
#include "Core/Archive.h"
#include "Core/Math/FRotator.h"

namespace Asset {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief 변환 섹션의 위치·회전·스케일 채널 이름입니다.
         */
        constexpr std::array<const char*, Asset::FSequenceTransformSection::ChannelCount> ChannelNames{"LocationX", "LocationY", "LocationZ", "RotationPitch", "RotationYaw", "RotationRoll", "ScaleX", "ScaleY", "ScaleZ"};
    }

    /**
     * @brief 시간이 섹션 구간에 포함되는지 검사합니다.
     */
    bool Asset::FSequenceTransformSection::Contains(double Tick) const {
        return Tick >= mStartTick && Tick <= mEndTick;
    }

    /**
     * @brief 지정한 시간이 포함되도록 섹션 구간을 넓힙니다.
     */
    void Asset::FSequenceTransformSection::ExpandToInclude(Core::Int32 Tick) {
        mStartTick = Tick < mStartTick ? Tick : mStartTick;
        mEndTick = Tick > mEndTick ? Tick : mEndTick;
    }

    /**
     * @brief 지정한 채널을 반환합니다.
     */
    Asset::FSequenceChannel& Asset::FSequenceTransformSection::GetChannel(Core::Index_t Channel) {
        return mChannels[Channel];
    }

    /**
     * @brief 채널의 저장 이름을 반환합니다.
     */
    const char* Asset::FSequenceTransformSection::GetChannelName(Core::Index_t Channel) {
        return ChannelNames[Channel];
    }

    /**
     * @brief 지정한 시간의 변환을 계산합니다.
     */
    Core::FTransform Asset::FSequenceTransformSection::Evaluate(double Tick, const Core::FTransform& Base) const {
        Core::FTransform Result{Base};
        const auto Sample{[this, Tick](Asset::ESequenceTransformChannel Channel, float& Value) {
            return mChannels[Channel].Evaluate(Tick, Value);
        }};

        Sample(Asset::ESequenceTransformChannel::LocationX, Result.mLocation.x);
        Sample(Asset::ESequenceTransformChannel::LocationY, Result.mLocation.y);
        Sample(Asset::ESequenceTransformChannel::LocationZ, Result.mLocation.z);
        Sample(Asset::ESequenceTransformChannel::ScaleX, Result.mScale.x);
        Sample(Asset::ESequenceTransformChannel::ScaleY, Result.mScale.y);
        Sample(Asset::ESequenceTransformChannel::ScaleZ, Result.mScale.z);

        // 회전 키가 없으면 오일러 변환 오차가 생기지 않도록 기준 쿼터니언을 유지합니다.
        Core::FRotator Rotation{Base.mRotation.Rotator()};
        bool Rotated{Sample(Asset::ESequenceTransformChannel::RotationPitch, Rotation.pitch)};

        Rotated = Sample(Asset::ESequenceTransformChannel::RotationYaw, Rotation.yaw) || Rotated;
        Rotated = Sample(Asset::ESequenceTransformChannel::RotationRoll, Rotation.roll) || Rotated;

        if (Rotated) {
            Result.mRotation = Rotation.Quaternion();
        }

        return Result;
    }

    /**
     * @brief 아카이브 모드에 따라 구간과 키가 있는 채널을 읽거나 저장합니다.
     */
    void Asset::FSequenceTransformSection::Serialize(Core::FArchive& Archive) {
        Archive.Serialize("StartTick", mStartTick);
        Archive.Serialize("EndTick", mEndTick);

        if (Archive.HasError()) {
            return;
        }

        if (mEndTick < mStartTick) {
            Archive.SetError("Invalid sequence section range");

            return;
        }

        if (!Archive.BeginObject("Channels")) {
            return;
        }

        for (Core::Index_t Index{}; Index < ChannelCount && !Archive.HasError(); ++Index) {
            if (Archive.IsLoading() ? !Archive.HasField(ChannelNames[Index]) : mChannels[Index].GetKeys().empty()) {
                continue;
            }

            if (Archive.BeginObject(ChannelNames[Index])) {
                mChannels[Index].Serialize(Archive);
                Archive.EndObject();
            }
        }

        Archive.EndObject();
    }

}
