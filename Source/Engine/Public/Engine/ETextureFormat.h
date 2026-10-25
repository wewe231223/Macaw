#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

enum class ETextureFormat : Uint8 {
    RGBA8,
    BGRA8,
    RGBA16Float,
    RGBA32Float,
    R32Float,
    D24S8,
    D32Float
};

ReflectEnum(ETextureFormat, RGBA8, BGRA8, RGBA16Float, RGBA32Float, R32Float, D24S8, D32Float);
