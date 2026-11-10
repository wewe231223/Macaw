#include "Asset/Animation/UBlendSpace.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Core/Archive.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace Asset {

    namespace {
        constexpr Core::Uint32 BlendSpaceFormatVersion{1};

        bool IsValidParameter(const Asset::FBlendParameter& Parameter) {
            return std::isfinite(Parameter.mMin) && std::isfinite(Parameter.mMax) && Parameter.mMin < Parameter.mMax && Parameter.mGridDivisions >= 1 && Parameter.mGridDivisions <= 100;
        }
    }

    const Asset::FBlendParameter& UBlendSpace::GetBlendParameter(Core::Uint32 Axis) const {
        return mBlendParameters[std::min<Core::Uint32>(Axis, 2)];
    }

    bool UBlendSpace::SetBlendParameter(Core::Uint32 Axis, const Asset::FBlendParameter& Parameter) {
        if (Axis >= GetNumAxes() || !IsValidParameter(Parameter)) {
            return false;
        }

        mBlendParameters[Axis] = Parameter;

        return true;
    }

    const Core::TArray<Asset::FBlendSample>& UBlendSpace::GetSamples() const {
        return mSamples;
    }

    Core::Int32 UBlendSpace::AddSample(const Core::FString& AnimationPath, const Asset::UAnimSequence* Animation, const Core::FVector& SampleValue) {
        if (mSamples.size() >= MaxSamples || !SampleValue.IsFinite()) {
            return -1;
        }

        mSamples.push_back({AnimationPath, Animation, SampleValue});

        return static_cast<Core::Int32>(mSamples.size() - 1);
    }

    bool UBlendSpace::RemoveSample(Core::Int32 Index) {
        if (Index < 0 || static_cast<std::size_t>(Index) >= mSamples.size()) {
            return false;
        }

        mSamples.erase(mSamples.begin() + Index);

        return true;
    }

    bool UBlendSpace::SetSampleAnimation(Core::Int32 Index, const Core::FString& AnimationPath, const Asset::UAnimSequence* Animation) {
        if (Index < 0 || static_cast<std::size_t>(Index) >= mSamples.size()) {
            return false;
        }

        mSamples[static_cast<std::size_t>(Index)].mAnimationPath = AnimationPath;
        mSamples[static_cast<std::size_t>(Index)].mAnimation = Animation;

        return true;
    }

    bool UBlendSpace::SetSampleValue(Core::Int32 Index, const Core::FVector& SampleValue) {
        if (Index < 0 || static_cast<std::size_t>(Index) >= mSamples.size() || !SampleValue.IsFinite()) {
            return false;
        }

        mSamples[static_cast<std::size_t>(Index)].mSampleValue = SampleValue;

        return true;
    }

    Core::FVector UBlendSpace::ClampInput(const Core::FVector& Input) const {
        std::array<float, 3> Values{Input.ToArray()};

        for (Core::Uint32 Axis{}; Axis < 3; ++Axis) {
            const float Value{std::isfinite(Values[Axis]) ? Values[Axis] : 0.0f};

            Values[Axis] = Axis < GetNumAxes() ? std::clamp(Value, mBlendParameters[Axis].mMin, mBlendParameters[Axis].mMax) : 0.0f;
        }

        return {Values[0], Values[1], Values[2]};
    }

    double UBlendSpace::GetBlendedPlayLength(const Core::TArray<Asset::FBlendSampleWeight>& Weights) const {
        double Length{};

        for (const Asset::FBlendSampleWeight& Weight : Weights) {
            if (Weight.mSampleIndex >= 0 && static_cast<std::size_t>(Weight.mSampleIndex) < mSamples.size()) {
                if (const Asset::UAnimSequence* Animation{mSamples[static_cast<std::size_t>(Weight.mSampleIndex)].mAnimation}) {
                    Length += Animation->GetPlayLength() * static_cast<double>(Weight.mWeight);
                }
            }
        }

        return Length;
    }

    double UBlendSpace::GetPlayLength() const {
        double Length{};

        for (const Asset::FBlendSample& Sample : mSamples) {
            if (Sample.mAnimation != nullptr) {
                Length = std::max(Length, Sample.mAnimation->GetPlayLength());
            }
        }

        return Length;
    }

    bool UBlendSpace::IsValid() const {
        return std::any_of(mSamples.begin(), mSamples.end(), [](const Asset::FBlendSample& Sample) {
            return Sample.mAnimation != nullptr && Sample.mAnimation->IsValid();
        });
    }

    void UBlendSpace::CopyFrom(const Asset::UBlendSpace& Source) {
        if (this != &Source) {
            std::copy_n(Source.mBlendParameters, std::size(mBlendParameters), mBlendParameters);
            mSamples = Source.mSamples;
        }
    }

    void UBlendSpace::Serialize(Core::FArchive& Archive) {
        Core::Uint32 Version{BlendSpaceFormatVersion};

        Archive.Serialize("FormatVersion", Version);

        if (Archive.HasError()) {
            return;
        }

        if (Version != BlendSpaceFormatVersion) {
            Archive.SetError("Unsupported blend space version");

            return;
        }

        Core::Uint32 Count{GetNumAxes()};

        if (!Archive.BeginArray("Axes", Count)) {
            return;
        }

        if (Archive.IsLoading() && Count != GetNumAxes()) {
            Archive.SetError("Blend space axis count does not match its type");
            Archive.EndArray();

            return;
        }

        for (Core::Uint32 Axis{}; Axis < Count && !Archive.HasError(); ++Axis) {
            if (!Archive.BeginObject(std::to_string(Axis))) {
                break;
            }

            Asset::FBlendParameter Parameter{mBlendParameters[Axis]};

            Archive.Serialize("Name", Parameter.mDisplayName);
            Archive.Serialize("Min", Parameter.mMin);
            Archive.Serialize("Max", Parameter.mMax);

            // 격자 칸 수는 나중에 추가한 값이므로 없는 파일은 기본값을 씁니다.
            if (!Archive.IsLoading() || Archive.HasField("Divisions")) {
                Archive.Serialize("Divisions", Parameter.mGridDivisions);
            }

            if (Archive.IsLoading() && !Archive.HasError()) {
                if (!IsValidParameter(Parameter)) {
                    Archive.SetError("Invalid blend space axis range");
                } else {
                    mBlendParameters[Axis] = Parameter;
                }
            }

            Archive.EndObject();
        }

        Archive.EndArray();

        if (Archive.HasError()) {
            return;
        }

        Count = static_cast<Core::Uint32>(mSamples.size());

        if (!Archive.BeginArray("Samples", Count)) {
            return;
        }

        if (Archive.IsLoading()) {
            if (Count > MaxSamples) {
                Archive.SetError("Too many blend space samples");
                Archive.EndArray();

                return;
            }

            mSamples.assign(Count, Asset::FBlendSample{});
        }

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            Asset::FBlendSample& Sample{mSamples[Index]};

            Archive.Serialize("Animation", Sample.mAnimationPath);
            Archive.Serialize("X", Sample.mSampleValue.x);
            Archive.Serialize("Y", Sample.mSampleValue.y);
            Archive.Serialize("Z", Sample.mSampleValue.z);

            if (Archive.IsLoading() && !Archive.HasError() && !Sample.mSampleValue.IsFinite()) {
                Archive.SetError("Invalid blend space sample value");
            }

            Archive.EndObject();
        }

        Archive.EndArray();
    }

}
