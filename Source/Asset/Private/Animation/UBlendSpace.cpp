/**
 * @file UBlendSpace.cpp
 * @brief 입력 좌표에 따라 샘플 애니메이션의 가중치를 계산하는 에셋의 기반입니다.
 * @ingroup Asset
 */

#include "Core/Types.h"
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

    /**
     * @brief 입력 좌표에 따라 샘플 애니메이션의 가중치를 계산하는 에셋의 기반의 파일 내부 보조 구현입니다.
     */
    namespace {

        /**
         * @brief 직렬화하는 블렌드 공간 파일 형식 버전입니다.
         */
        constexpr Core::Uint32 BlendSpaceFormatVersion{1};

        /**
         * @brief 블렌드 축의 이름·범위·격자 설정이 유효한지 확인합니다.
         * @param Parameter 조회하거나 연결할 파라미터 이름 또는 인덱스입니다.
         * @return 유한하고 일관된 파라미터 설정이면 true입니다.
         */
        bool IsValidParameter(const Asset::FBlendParameter& Parameter) {
            return std::isfinite(Parameter.mMin) && std::isfinite(Parameter.mMax) && Parameter.mMin < Parameter.mMax && Parameter.mGridDivisions >= 1 && Parameter.mGridDivisions <= 100;
        }
    }

    const Asset::FBlendParameter& UBlendSpace::GetBlendParameter(Asset::EBlendSpaceAxis Axis) const {
        const auto AxisIndex{static_cast<Core::Index_t>(Axis)};

        return mBlendParameters[std::clamp<Core::Index_t>(AxisIndex, 0, static_cast<Core::Index_t>(std::size(mBlendParameters)) - 1)];
    }

    bool UBlendSpace::SetBlendParameter(Asset::EBlendSpaceAxis Axis, const Asset::FBlendParameter& Parameter) {
        const auto AxisIndex{static_cast<Core::Index_t>(Axis)};

        if (AxisIndex < 0 || AxisIndex >= GetNumAxes() || AxisIndex >= std::size(mBlendParameters) || !IsValidParameter(Parameter)) {
            return false;
        }

        mBlendParameters[AxisIndex] = Parameter;

        return true;
    }

    const Core::TArray<Asset::FBlendSample>& UBlendSpace::GetSamples() const {
        return mSamples;
    }

    Core::Index_t UBlendSpace::AddSample(const Asset::UAnimSequence* Animation, const Core::FVector& SampleValue) {
        if (mSamples.size() >= MaxSamples || !SampleValue.IsFinite()) {
            return -1;
        }

        mSamples.push_back({Animation, SampleValue});
        RebuildSampleOrder();

        return static_cast<Core::Index_t>(mSamples.size() - 1);
    }

    bool UBlendSpace::RemoveSample(Core::Index_t Index) {
        if (Index < 0 || Index >= mSamples.size()) {
            return false;
        }

        mSamples.erase(mSamples.begin() + Index);
        RebuildSampleOrder();

        return true;
    }

    bool UBlendSpace::SetSampleAnimation(Core::Index_t Index, const Asset::UAnimSequence* Animation) {
        if (Index < 0 || Index >= mSamples.size()) {
            return false;
        }

        mSamples[Index].mAnimation = Animation;

        return true;
    }

    bool UBlendSpace::SetSampleValue(Core::Index_t Index, const Core::FVector& SampleValue) {
        if (Index < 0 || Index >= mSamples.size() || !SampleValue.IsFinite()) {
            return false;
        }

        mSamples[Index].mSampleValue = SampleValue;
        RebuildSampleOrder();

        return true;
    }

    Core::FVector UBlendSpace::ClampInput(const Core::FVector& Input) const {
        std::array<float, 3> Values{Input.ToArray()};

        for (Core::Index_t AxisIndex{}; AxisIndex < Values.size(); ++AxisIndex) {
            const float Value{std::isfinite(Values[AxisIndex]) ? Values[AxisIndex] : 0.0f};

            Values[AxisIndex] = AxisIndex < GetNumAxes() ? std::clamp(Value, mBlendParameters[AxisIndex].mMin, mBlendParameters[AxisIndex].mMax) : 0.0f;
        }

        return {Values[0], Values[1], Values[2]};
    }

    double UBlendSpace::GetBlendedPlayLength(const Core::TArray<Asset::FBlendSampleWeight>& Weights) const {
        double Length{};

        for (const Asset::FBlendSampleWeight& Weight : Weights) {
            if (Weight.mSampleIndex >= 0 && static_cast<Core::Index_t>(Weight.mSampleIndex) < mSamples.size()) {
                if (const Asset::UAnimSequence* Animation{mSamples[static_cast<Core::Index_t>(Weight.mSampleIndex)].mAnimation}) {
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

        for (Core::Index_t AxisIndex{}; AxisIndex < Count && !Archive.HasError(); ++AxisIndex) {
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

        for (Core::Index_t Index{}; Index < Count && !Archive.HasError(); ++Index) {
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
