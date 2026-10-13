#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include "Core/Common.h"

enum class EMaterialBlendMode : Uint8 {
    Opaque,
    Translucent
};

ReflectEnum(EMaterialBlendMode, ReflectEnumValue(EMaterialBlendMode, Opaque), ReflectEnumValue(EMaterialBlendMode, Translucent));
