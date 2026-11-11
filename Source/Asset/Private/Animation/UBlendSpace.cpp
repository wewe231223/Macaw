#include "Asset/Animation/UBlendSpace.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Asset/FAssetManager.h"
#include "Core/Archive.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/FilePath.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <string>
#include <utility>

namespace Asset {

    namespace {
        constexpr Core::Uint32 BlendSpaceFormatVersion{1};

        bool IsValidParameter(const Asset::FBlendParameter& Parameter) {
            return std::isfinite(Parameter.mMin) && std::isfinite(Parameter.mMax) && Parameter.mMin < Parameter.mMax && Parameter.mGridDivisions >= 1 && Parameter.mGridDivisions <= 100;
        }
    }

    const Asset::FBlendParameter& UBlendSpace::GetBlendParameter(Asset::EBlendSpaceAxis Axis) const {
        const auto AxisIndex{static_cast<std::size_t>(Axis)};

        return mBlendParameters[std::min(AxisIndex, std::size(mBlendParameters) - 1)];
    }

    bool UBlendSpace::SetBlendParameter(Asset::EBlendSpaceAxis Axis, const Asset::FBlendParameter& Parameter) {
        const auto AxisIndex{static_cast<std::size_t>(Axis)};

        if (AxisIndex >= GetNumAxes() || AxisIndex >= std::size(mBlendParameters) || !IsValidParameter(Parameter)) {
            return false;
        }

        mBlendParameters[AxisIndex] = Parameter;

        return true;
    }

    const Core::TArray<Asset::FBlendSample>& UBlendSpace::GetSamples() const {
        return mSamples;
    }

    Core::Int32 UBlendSpace::AddSample(const Asset::UAnimSequence* Animation, const Core::FVector& SampleValue) {
        if (mSamples.size() >= MaxSamples || !SampleValue.IsFinite()) {
            return -1;
        }

        mSamples.push_back({Animation, SampleValue});
        RebuildSampleOrder();

        return static_cast<Core::Int32>(mSamples.size() - 1);
    }

    bool UBlendSpace::RemoveSample(Core::Int32 Index) {
        if (Index < 0 || static_cast<std::size_t>(Index) >= mSamples.size()) {
            return false;
        }

        mSamples.erase(mSamples.begin() + Index);
        RebuildSampleOrder();

        return true;
    }

    bool UBlendSpace::SetSampleAnimation(Core::Int32 Index, const Asset::UAnimSequence* Animation) {
        if (Index < 0 || static_cast<std::size_t>(Index) >= mSamples.size()) {
            return false;
        }

        mSamples[static_cast<std::size_t>(Index)].mAnimation = Animation;

        return true;
    }

    bool UBlendSpace::SetSampleValue(Core::Int32 Index, const Core::FVector& SampleValue) {
        if (Index < 0 || static_cast<std::size_t>(Index) >= mSamples.size() || !SampleValue.IsFinite()) {
            return false;
        }

        mSamples[static_cast<std::size_t>(Index)].mSampleValue = SampleValue;
        RebuildSampleOrder();

        return true;
    }

    Core::FVector UBlendSpace::ClampInput(const Core::FVector& Input) const {
        std::array<float, 3> Values{Input.ToArray()};

        for (std::size_t AxisIndex{}; AxisIndex < Values.size(); ++AxisIndex) {
            const float Value{std::isfinite(Values[AxisIndex]) ? Values[AxisIndex] : 0.0f};

            Values[AxisIndex] = AxisIndex < GetNumAxes() ? std::clamp(Value, mBlendParameters[AxisIndex].mMin, mBlendParameters[AxisIndex].mMax) : 0.0f;
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
            auto Samples{Source.mSamples};

            std::copy_n(Source.mBlendParameters, std::size(mBlendParameters), mBlendParameters);
            mSamples = std::move(Samples);
            RebuildSampleOrder();
        }
    }

    void UBlendSpace::Serialize(Core::FArchive& Archive, Asset::FAssetManager& Assets) {
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

        for (Core::Uint32 AxisIndex{}; AxisIndex < Count && !Archive.HasError(); ++AxisIndex) {
            if (!Archive.BeginObject(std::to_string(AxisIndex))) {
                break;
            }

            Asset::FBlendParameter Parameter{mBlendParameters[AxisIndex]};

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
                    mBlendParameters[AxisIndex] = Parameter;
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

        Core::TArray<Asset::FBlendSample> LoadedSamples{};

        if (Archive.IsLoading()) {
            if (Count > MaxSamples) {
                Archive.SetError("Too many blend space samples");
                Archive.EndArray();

                return;
            }

            LoadedSamples.resize(Count);
        }

        auto& Samples{Archive.IsLoading() ? LoadedSamples : mSamples};

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            Asset::FBlendSample& Sample{Samples[Index]};

            Core::FString AnimationPath{Sample.mAnimation != nullptr ? Sample.mAnimation->GetAssetPath() : Core::FString{}};
            Archive.Serialize("Animation", AnimationPath);
            Archive.Serialize("X", Sample.mSampleValue.x);
            Archive.Serialize("Y", Sample.mSampleValue.y);
            Archive.Serialize("Z", Sample.mSampleValue.z);

            if (Archive.IsLoading() && !Archive.HasError()) {
                if (!Sample.mSampleValue.IsFinite()) {
                    Archive.SetError("Invalid blend space sample value");
                } else if (!AnimationPath.empty()) {
                    Core::FString AnimationError{};
                    Sample.mAnimation = Assets.LoadAnimSequence(Core::FilePath::FromUtf8(AnimationPath), AnimationError);

                    if (Sample.mAnimation == nullptr) {
                        Core::ErrorHandler::Report("BlendSpace", "Cannot load sample animation '" + AnimationPath + "' of blend space '" + GetName() + "': " + AnimationError, Core::ErrorHandler::EErrorLevel::Warning);
                    }
                }
            }

            Archive.EndObject();
        }

        Archive.EndArray();

        if (Archive.IsLoading() && !Archive.HasError()) {
            mSamples = std::move(LoadedSamples);
            RebuildSampleOrder();
        }
    }

}
