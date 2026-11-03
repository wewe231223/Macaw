#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector4.h"

namespace Renderer {

    struct FGpuLight {
    public:
        Core::FVector4 mPositionAndRadius{};
        Core::FVector4 mDirectionAndType{};
        Core::FVector4 mColorAndIntensity{};
        Core::FVector4 mFalloffAndCones{};
        Core::FVector4 mShadowInfo{};
    };

}
