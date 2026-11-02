#include "Asset/Sequence/FSequenceChannel.h"
#include "Core/Archive.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace Asset {

    namespace {
        constexpr Core::Uint32 MaximumKeyCount{1000000};

        const char* ToInterpolationName(Asset::ESequenceInterpolation Interpolation) {
            switch (Interpolation) {
            case Asset::ESequenceInterpolation::Constant:
                return "Constant";
            case Asset::ESequenceInterpolation::Linear:
                return "Linear";
            default:
                return "Cubic";
            }
        }

        bool ParseInterpolation(const Core::FString& Name, Asset::ESequenceInterpolation& Interpolation) {
            if (Name == "Constant") {
                Interpolation = Asset::ESequenceInterpolation::Constant;
            } else if (Name == "Linear") {
                Interpolation = Asset::ESequenceInterpolation::Linear;
            } else if (Name == "Cubic") {
                Interpolation = Asset::ESequenceInterpolation::Cubic;
            } else {
                return false;
            }

            return true;
        }

        // 양 끝 키는 평평하게, 내부 키는 이웃 키의 기울기로 탄젠트를 계산합니다.
        double GetAutoTangent(const Core::TArray<Asset::FSequenceKey>& Keys, std::size_t Index) {
            if (Index == 0 || Index + 1 >= Keys.size()) {
                return 0.0;
            }

            const Asset::FSequenceKey& Previous{Keys[Index - 1]};
            const Asset::FSequenceKey& Next{Keys[Index + 1]};

            return (static_cast<double>(Next.mValue) - Previous.mValue) / (static_cast<double>(Next.mTick) - Previous.mTick);
        }
    }

    const Core::TArray<Asset::FSequenceKey>& Asset::FSequenceChannel::GetKeys() const {
        return mKeys;
    }

    bool Asset::FSequenceChannel::AddKey(const Asset::FSequenceKey& Key) {
        if (!std::isfinite(Key.mValue)) {
            return false;
        }

        const auto Position{std::lower_bound(mKeys.begin(), mKeys.end(), Key.mTick, [](const Asset::FSequenceKey& Existing, Core::Int32 Tick) {
            return Existing.mTick < Tick;
        })};

        if (Position != mKeys.end() && Position->mTick == Key.mTick) {
            *Position = Key;
        } else {
            mKeys.insert(Position, Key);
        }

        return true;
    }

    bool Asset::FSequenceChannel::RemoveKey(Core::Int32 Tick) {
        return std::erase_if(mKeys, [Tick](const Asset::FSequenceKey& Key) {
            return Key.mTick == Tick;
        }) > 0;
    }

    bool Asset::FSequenceChannel::MoveKey(Core::Int32 FromTick, Core::Int32 ToTick) {
        const auto HasTick{[](Core::Int32 Tick) {
            return [Tick](const Asset::FSequenceKey& Key) {
                return Key.mTick == Tick;
            };
        }};
        const auto Source{std::find_if(mKeys.begin(), mKeys.end(), HasTick(FromTick))};

        if (Source == mKeys.end() || std::any_of(mKeys.begin(), mKeys.end(), HasTick(ToTick))) {
            return false;
        }

        Asset::FSequenceKey Key{*Source};
        Key.mTick = ToTick;
        mKeys.erase(Source);

        return AddKey(Key);
    }

    bool Asset::FSequenceChannel::Evaluate(double Tick, float& Value) const {
        if (mKeys.empty()) {
            return false;
        }

        if (Tick <= mKeys.front().mTick) {
            Value = mKeys.front().mValue;
            return true;
        }

        if (Tick >= mKeys.back().mTick) {
            Value = mKeys.back().mValue;
            return true;
        }

        const auto Next{std::upper_bound(mKeys.begin(), mKeys.end(), Tick, [](double Time, const Asset::FSequenceKey& Key) {
            return Time < Key.mTick;
        })};
        const std::size_t Index{static_cast<std::size_t>(Next - mKeys.begin()) - 1};
        const Asset::FSequenceKey& From{mKeys[Index]};
        const Asset::FSequenceKey& To{*Next};
        const double Span{static_cast<double>(To.mTick) - From.mTick};
        const double Alpha{(Tick - From.mTick) / Span};

        switch (From.mInterpolation) {
        case Asset::ESequenceInterpolation::Constant:
            Value = From.mValue;
            break;
        case Asset::ESequenceInterpolation::Linear:
            Value = static_cast<float>(From.mValue + (static_cast<double>(To.mValue) - From.mValue) * Alpha);
            break;
        default: {
            const double Alpha2{Alpha * Alpha};
            const double Alpha3{Alpha2 * Alpha};
            Value = static_cast<float>((2.0 * Alpha3 - 3.0 * Alpha2 + 1.0) * From.mValue + (Alpha3 - 2.0 * Alpha2 + Alpha) * Span * GetAutoTangent(mKeys, Index) + (-2.0 * Alpha3 + 3.0 * Alpha2) * To.mValue + (Alpha3 - Alpha2) * Span * GetAutoTangent(mKeys, Index + 1));
            break;
        }
        }

        return true;
    }

    void Asset::FSequenceChannel::Serialize(Core::FArchive& Archive) {
        Core::Uint32 Count{static_cast<Core::Uint32>(mKeys.size())};

        if (!Archive.BeginArray("Keys", Count)) {
            return;
        }

        if (Count > MaximumKeyCount) {
            Archive.SetError("Invalid sequence key count");
        }

        if (Archive.IsLoading()) {
            mKeys.clear();
        }

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            Asset::FSequenceKey Key{Archive.IsLoading() ? Asset::FSequenceKey{} : mKeys[Index]};
            Core::FString Interpolation{ToInterpolationName(Key.mInterpolation)};
            Archive.Serialize("Tick", Key.mTick);
            Archive.Serialize("Value", Key.mValue);
            Archive.Serialize("Interpolation", Interpolation);

            if (Archive.IsLoading() && !Archive.HasError() && (!ParseInterpolation(Interpolation, Key.mInterpolation) || !AddKey(Key))) {
                Archive.SetError("Invalid sequence key");
            }

            Archive.EndObject();
        }

        Archive.EndArray();
    }

}
