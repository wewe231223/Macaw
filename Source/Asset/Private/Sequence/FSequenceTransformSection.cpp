#include "Asset/Sequence/FSequenceTransformSection.h"
#include "Core/Archive.h"
#include "Core/Math/FRotator.h"

namespace Asset {

    namespace {
        constexpr std::array<const char*, Asset::FSequenceTransformSection::ChannelCount> ChannelNames{"LocationX", "LocationY", "LocationZ", "RotationPitch", "RotationYaw", "RotationRoll", "ScaleX", "ScaleY", "ScaleZ"};
    }

    bool Asset::FSequenceTransformSection::Contains(double Tick) const {
        return Tick >= mStartTick && Tick <= mEndTick;
    }

    void Asset::FSequenceTransformSection::ExpandToInclude(Core::Int32 Tick) {
        mStartTick = Tick < mStartTick ? Tick : mStartTick;
        mEndTick = Tick > mEndTick ? Tick : mEndTick;
    }

    Asset::FSequenceChannel& Asset::FSequenceTransformSection::GetChannel(std::size_t Channel) {
        return mChannels[Channel];
    }

    const char* Asset::FSequenceTransformSection::GetChannelName(std::size_t Channel) {
        return ChannelNames[Channel];
    }

    Core::FTransform Asset::FSequenceTransformSection::Evaluate(double Tick, const Core::FTransform& Base) const {
        Core::FTransform Result{Base};
        const auto Sample{[this, Tick](Asset::ESequenceTransformChannel Channel, float& Value) {
            return mChannels[static_cast<std::size_t>(Channel)].Evaluate(Tick, Value);
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

        for (std::size_t Index{}; Index < ChannelCount && !Archive.HasError(); ++Index) {
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
