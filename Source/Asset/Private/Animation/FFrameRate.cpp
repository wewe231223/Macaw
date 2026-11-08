#include "Asset/Animation/FFrameRate.h"

#include <cmath>

namespace Asset {

    Asset::FFrameRate Asset::FFrameRate::FromDecimal(double FramesPerSecond) {
        constexpr double Tolerance{0.0001};
        constexpr double MaxRate{100000.0};

        if (!std::isfinite(FramesPerSecond) || FramesPerSecond <= 0.0 || FramesPerSecond > MaxRate) {
            return {0, 1};
        }

        const double Rounded{std::round(FramesPerSecond)};

        if (std::abs(FramesPerSecond - Rounded) <= Tolerance) {
            return {static_cast<Core::Uint32>(Rounded), 1};
        }

        // 23.976, 29.97, 59.94 같은 NTSC 계열은 N * 1000 / 1001입니다.
        const double Ntsc{FramesPerSecond * 1001.0};

        if (std::abs(Ntsc - std::round(Ntsc)) <= Tolerance * 1001.0) {
            return {static_cast<Core::Uint32>(std::round(Ntsc)), 1001};
        }

        return {static_cast<Core::Uint32>(std::round(FramesPerSecond * 1000.0)), 1000};
    }

    double Asset::FFrameRate::AsDecimal() const {
        return mDenominator == 0 ? 0.0 : static_cast<double>(mNumerator) / static_cast<double>(mDenominator);
    }

    double Asset::FFrameRate::AsInterval() const {
        return mNumerator == 0 ? 0.0 : static_cast<double>(mDenominator) / static_cast<double>(mNumerator);
    }

    double Asset::FFrameRate::AsSeconds(double Frame) const {
        return Frame * AsInterval();
    }

    bool Asset::FFrameRate::IsValid() const {
        return mNumerator != 0 && mDenominator != 0;
    }

}
