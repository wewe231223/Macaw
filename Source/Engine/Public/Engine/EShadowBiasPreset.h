#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "EShadowBiasPreset.generated.h"

namespace Engine {

    UENUM()
    enum class EShadowBiasPreset : Core::Uint8 {
        Level0 = 0,
        Level1,
        Level2,
        Level3,
        Level4,
        Level5,
        Level6,
        Level7,
        Level8,
        Level9,
        Level10,
        Level11,
        Level12,
        Level13,
        Level14,
        Level15
    };

    inline constexpr Core::Uint32 ShadowBiasPresetCount{static_cast<Core::Uint32>(Engine::EShadowBiasPreset::Level15) + 1u};
    inline constexpr Engine::EShadowBiasPreset DefaultShadowBiasPreset{Engine::EShadowBiasPreset::Level4};
    inline constexpr Core::Int32 ShadowDepthBiasStep{512};
    inline constexpr float ShadowSlopeBiasStep{0.25f};

}
