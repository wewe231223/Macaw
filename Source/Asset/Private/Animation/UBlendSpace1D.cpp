/**
 * @file UBlendSpace1D.cpp
 * @brief X 축 하나에 샘플을 늘어놓고, 입력 양옆의 두 샘플을 거리 비율로 섞습니다.
 * @ingroup Asset
 */

#include "Core/Types.h"
#include "Asset/Animation/UBlendSpace1D.h"
#include "Asset/Animation/UAnimSequence.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>
#include <span>
#include <utility>

namespace Asset {

    void UBlendSpace1D::GetSamplesToBlend(const Core::FVector& Input, Core::TArray<Asset::FBlendSampleWeight>& OutWeights) const {
        OutWeights.clear();

        const auto& Samples{GetSamples()};
        const std::span<const Core::Int32> Sorted{mSortedSampleIndices.data(), Samples.size()};
        const auto IsUsable{[&Samples](Core::Int32 Index) {
            const Asset::UAnimSequence* Animation{Samples[Index].mAnimation};

            return Animation != nullptr && Animation->IsValid();
        }};

        const auto First{std::ranges::find_if(Sorted, IsUsable)};

        if (First == Sorted.end()) {
            return;
        }

        const auto SampleX{[&Samples](Core::Int32 Index) {
            return Samples[Index].mSampleValue.x;
        }};

        const float X{ClampInput(Input).x};

        if (X <= SampleX(*First)) {
            OutWeights.push_back({*First, 1.0f});

            return;
        }

        const auto Last{std::find_if(Sorted.rbegin(), Sorted.rend(), IsUsable)};

        if (X >= SampleX(*Last)) {
            OutWeights.push_back({*Last, 1.0f});

            return;
        }

        // X를 사이에 둔 두 샘플을 찾아 거리 비율로 섞습니다. 앞의 두 검사로 X는 처음과 마지막 샘플 사이에 있습니다.
        const auto Split{std::ranges::lower_bound(First, Last.base(), X, std::ranges::less{}, SampleX)};
        const Core::Int32 Low{static_cast<Core::Int32>(*std::find_if(std::make_reverse_iterator(Split), Sorted.rend(), IsUsable))};
        const Core::Int32 High{static_cast<Core::Int32>(*std::find_if(Split, Last.base(), IsUsable))};
        const float LowX{SampleX(Low)};
        const float Range{SampleX(High) - LowX};
        const float Alpha{Range > 0.0f ? std::clamp((X - LowX) / Range, 0.0f, 1.0f) : 0.0f};

        if (Alpha < 1.0f) {
            OutWeights.push_back({Low, 1.0f - Alpha});
        }

        if (Alpha > 0.0f) {
            OutWeights.push_back({High, Alpha});
        }
    }

    Core::Uint32 UBlendSpace1D::GetNumAxes() const {
        return 1;
    }

    void UBlendSpace1D::RebuildSampleOrder() {
        const auto& Samples{GetSamples()};
        const std::span<Core::Int32> Sorted{mSortedSampleIndices.data(), Samples.size()};

        std::iota(Sorted.begin(), Sorted.end(), Core::Int32{});
        std::ranges::sort(Sorted, std::ranges::less{}, [&Samples](Core::Int32 Index) {
            return std::pair{Samples[Index].mSampleValue.x, Index};
        });
    }

}
