/**
 * @file EShadowBiasPreset.h
 * @brief 깊이와 경사 바이어스를 단계별로 선택하는 그림자 프리셋입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "EShadowBiasPreset.generated.h"

namespace Engine {

    /**
     * @brief 깊이와 경사 바이어스를 단계별로 선택하는 그림자 프리셋입니다.
     * @ingroup Engine
     */
    UENUM()
    enum class EShadowBiasPreset : Core::Uint8 {

        /**
         * @brief 그림자 바이어스 0단계입니다.
         */
        Level0 = 0,

        /**
         * @brief 그림자 바이어스 1단계입니다.
         */
        Level1,

        /**
         * @brief 그림자 바이어스 2단계입니다.
         */
        Level2,

        /**
         * @brief 그림자 바이어스 3단계입니다.
         */
        Level3,

        /**
         * @brief 그림자 바이어스 4단계입니다.
         */
        Level4,

        /**
         * @brief 그림자 바이어스 5단계입니다.
         */
        Level5,

        /**
         * @brief 그림자 바이어스 6단계입니다.
         */
        Level6,

        /**
         * @brief 그림자 바이어스 7단계입니다.
         */
        Level7,

        /**
         * @brief 그림자 바이어스 8단계입니다.
         */
        Level8,

        /**
         * @brief 그림자 바이어스 9단계입니다.
         */
        Level9,

        /**
         * @brief 그림자 바이어스 10단계입니다.
         */
        Level10,

        /**
         * @brief 그림자 바이어스 11단계입니다.
         */
        Level11,

        /**
         * @brief 그림자 바이어스 12단계입니다.
         */
        Level12,

        /**
         * @brief 그림자 바이어스 13단계입니다.
         */
        Level13,

        /**
         * @brief 그림자 바이어스 14단계입니다.
         */
        Level14,

        /**
         * @brief 그림자 바이어스 15단계입니다.
         */
        Level15
    };

    /**
     * @brief 그림자 바이어스 프리셋 단계 수입니다.
     */
    inline constexpr Core::Uint32 ShadowBiasPresetCount{static_cast<Core::Uint32>(Engine::EShadowBiasPreset::Level15) + 1u};

    /**
     * @brief 조명의 기본 그림자 바이어스 단계입니다.
     */
    inline constexpr Engine::EShadowBiasPreset DefaultShadowBiasPreset{Engine::EShadowBiasPreset::Level4};

    /**
     * @brief 프리셋 한 단계당 정수 깊이 바이어스 증분입니다.
     */
    inline constexpr Core::Int32 ShadowDepthBiasStep{512};

    /**
     * @brief 프리셋 한 단계당 경사 바이어스 증분입니다.
     */
    inline constexpr float ShadowSlopeBiasStep{0.25f};

}
