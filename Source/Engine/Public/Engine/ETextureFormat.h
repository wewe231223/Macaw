#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

enum class ETextureFormat : Uint8 {
    RGBA8,
    BGRA8
};

ReflectEnum(ETextureFormat, RGBA8, BGRA8);
