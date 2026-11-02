#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector4.h"

namespace Renderer {

    struct FGpuLight {
    public:
        ReflectTypes(FGpuLight);

    public:
        ReflectStruct(FGpuLight);

    private:
        ReflectMembers();

    public:
        Core::FVector4 mPositionAndRadius{};
        ReflectProperty(mPositionAndRadius, "PositionAndRadius", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        Core::FVector4 mDirectionAndType{};
        ReflectProperty(mDirectionAndType, "DirectionAndType", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        Core::FVector4 mColorAndIntensity{};
        ReflectProperty(mColorAndIntensity, "ColorAndIntensity", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        Core::FVector4 mFalloffAndCones{};
        ReflectProperty(mFalloffAndCones, "FalloffAndCones", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        Core::FVector4 mShadowInfo{};
        ReflectProperty(mShadowInfo, "ShadowInfo", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
