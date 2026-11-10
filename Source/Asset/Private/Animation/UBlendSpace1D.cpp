#include "Asset/Animation/UBlendSpace1D.h"
#include "Asset/Animation/UAnimSequence.h"

#include <algorithm>

namespace Asset {

    void UBlendSpace1D::GetSamplesToBlend(const Core::FVector& Input, Core::TArray<Asset::FBlendSampleWeight>& OutWeights) const {
        OutWeights.clear();

        // 시퀀스가 해석된 샘플만 X 오름차순으로 늘어놓습니다. 안정 정렬이므로 X가 같으면 먼저 추가한 샘플이 앞에 옵니다.
        Core::TArray<Core::Int32> Sorted{};

        Sorted.reserve(mSamples.size());

        for (std::size_t Index{}; Index < mSamples.size(); ++Index) {
            if (mSamples[Index].mAnimation != nullptr && mSamples[Index].mAnimation->IsValid()) {
                Sorted.push_back(static_cast<Core::Int32>(Index));
            }
        }

        if (Sorted.empty()) {
            return;
        }

        std::stable_sort(Sorted.begin(), Sorted.end(), [this](Core::Int32 Left, Core::Int32 Right) {
            return mSamples[static_cast<std::size_t>(Left)].mSampleValue.x < mSamples[static_cast<std::size_t>(Right)].mSampleValue.x;
        });

        const float X{ClampInput(Input).x};
        const auto ValueOf{[this](Core::Int32 Index) {
            return mSamples[static_cast<std::size_t>(Index)].mSampleValue.x;
        }};

        if (X <= ValueOf(Sorted.front())) {
            OutWeights.push_back({Sorted.front(), 1.0f});

            return;
        }

        if (X >= ValueOf(Sorted.back())) {
            OutWeights.push_back({Sorted.back(), 1.0f});

            return;
        }

        // X를 사이에 둔 두 샘플을 찾아 거리 비율로 섞습니다. 앞의 두 검사로 X는 처음과 마지막 샘플 사이에 있습니다.
        std::size_t Upper{1};

        while (Upper + 1 < Sorted.size() && ValueOf(Sorted[Upper]) < X) {
            ++Upper;
        }

        const Core::Int32 Low{Sorted[Upper - 1]};
        const Core::Int32 High{Sorted[Upper]};
        const float Range{ValueOf(High) - ValueOf(Low)};
        const float Alpha{Range > 0.0f ? std::clamp((X - ValueOf(Low)) / Range, 0.0f, 1.0f) : 0.0f};

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

}
