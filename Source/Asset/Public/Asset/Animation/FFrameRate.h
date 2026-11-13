/**
 * @file FFrameRate.h
 * @brief 분수로 표현한 애니메이션 샘플레이트입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"

namespace Asset {

    /**
     * @brief 초당 프레임 수를 Numerator / Denominator 분수로 보관합니다.
     * @details 언리얼 FFrameRate와 같이 29.97(30000 / 1001)처럼 정수가 아닌 레이트도 정확히 나타냅니다.
     */
    struct FFrameRate {
    public:

        /**
         * @brief 초당 프레임 수를 실수로 근사한 분수로 만듭니다.
         * @param[in] FramesPerSecond 양수인 초당 프레임 수입니다.
         * @return 정수 레이트, NTSC 계열(분모 1001), 그 밖에는 분모 1000의 분수이며, 입력이 유효하지 않으면 0 / 1입니다.
         */
        static Asset::FFrameRate FromDecimal(double FramesPerSecond);

        /**
         * @brief 초당 프레임 수를 실수로 반환합니다.
         * @return Numerator / Denominator이며 분모가 0이면 0입니다.
         */
        double AsDecimal() const;

        /**
         * @brief 한 프레임의 길이를 초 단위로 반환합니다.
         * @return Denominator / Numerator이며 분자가 0이면 0입니다.
         */
        double AsInterval() const;

        /**
         * @brief 프레임 번호를 초 단위 시간으로 바꿉니다.
         * @param[in] Frame 프레임 번호입니다.
         * @return 0번 프레임 기준 시간입니다.
         */
        double AsSeconds(double Frame) const;

        /**
         * @brief 분자와 분모가 모두 양수인지 검사합니다.
         * @return 사용할 수 있는 레이트이면 true입니다.
         */
        bool IsValid() const;

        /**
         * @brief 프레임률의 분자와 분모가 같은지 비교합니다.
         * @param Other 비교하거나 복사할 상대 객체입니다.
         * @return 두 프레임률의 저장 값이 같으면 true입니다.
         */
        bool operator==(const Asset::FFrameRate& Other) const = default;

    public:

        /**
         * @brief 분자입니다. 정수 레이트에서는 초당 프레임 수입니다.
         */
        Core::Uint32 mNumerator{30};

        /**
         * @brief 분모입니다.
         */
        Core::Uint32 mDenominator{1};
    };

}
