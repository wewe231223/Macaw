/**
 * @file FSequenceKey.h
 * @brief 시퀀스 채널의 단일 키에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Sequence/ESequenceInterpolation.h"
#include "Core/ReflectionMacros.h"

namespace Asset {

    /**
     * @brief 시퀀스 채널의 단일 키를 저장합니다.
     * @ingroup Asset
     */
    struct FSequenceKey {
    public:
        /**
         * @brief 키의 시간이며 ULevelSequence::TicksPerSecond 단위입니다.
         */
        Core::Int32 mTick{};

        /**
         * @brief 키의 값입니다.
         */
        float mValue{};

        /**
         * @brief 이 키에서 다음 키까지의 보간 방식입니다.
         */
        Asset::ESequenceInterpolation mInterpolation{Asset::ESequenceInterpolation::Cubic};
    };

}
