/**
 * @file FSequenceChannel.cpp
 * @brief 시간순 키에서 단일 float 값을 계산하는 채널에 대한 선언입니다.
 * @ingroup Asset
 */

#include "Asset/Sequence/FSequenceChannel.h"
#include "Core/Archive.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace Asset {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief 파일에서 허용할 채널별 최대 키 수입니다.
         */
        constexpr Core::Uint32 MaximumKeyCount{1000000};

        /**
         * @brief 보간 종류를 파일에 저장할 이름으로 변환합니다.
         * @param[in] Interpolation 변환할 보간 종류입니다.
         * @return 파일에 저장할 보간 종류 이름입니다.
         */
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

        /**
         * @brief 저장된 보간 이름을 시퀀스 보간 종류로 해석합니다.
         * @param[in] Name 조회·등록·출력할 대상 이름입니다.
         * @param[in] Interpolation 변환할 보간 종류입니다.
         * @return 지원하는 보간 이름을 해석했으면 true입니다.
         */
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

        /**
         * @brief 인접 키의 값과 틱 간격에서 자동 보간 접선을 계산합니다.
         * @param[in] Keys 키 상태를 읽을 비트 집합입니다.
         * @param[in] Index 처리할 원소 또는 토큰의 인덱스입니다.
         * @return 인접 키의 값 변화량을 틱 간격으로 나눈 접선입니다.
         */
        double GetAutoTangent(const Core::TArray<Asset::FSequenceKey>& Keys, std::size_t Index) {
            if (Index == 0 || Index + 1 >= Keys.size()) {
                return 0.0;
            }

            const Asset::FSequenceKey& Previous{Keys[Index - 1]};
            const Asset::FSequenceKey& Next{Keys[Index + 1]};

            return (static_cast<double>(Next.mValue) - Previous.mValue) / (static_cast<double>(Next.mTick) - Previous.mTick);
        }
    }

    /**
     * @brief 시간순으로 정렬된 키 목록을 반환합니다.
     */
    const Core::TArray<Asset::FSequenceKey>& Asset::FSequenceChannel::GetKeys() const {
        return mKeys;
    }

    /**
     * @brief 키를 시간순 위치에 추가하고 같은 시간의 키는 교체합니다.
     */
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

    /**
     * @brief 지정한 시간의 키를 제거합니다.
     */
    bool Asset::FSequenceChannel::RemoveKey(Core::Int32 Tick) {
        return std::erase_if(mKeys, [Tick](const Asset::FSequenceKey& Key) {
            return Key.mTick == Tick;
        }) > 0;
    }

    /**
     * @brief 키를 다른 시간으로 옮깁니다.
     */
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

    /**
     * @brief 지정한 시간의 값을 계산합니다.
     */
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

    /**
     * @brief 아카이브 모드에 따라 키 목록을 읽거나 저장합니다.
     */
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
