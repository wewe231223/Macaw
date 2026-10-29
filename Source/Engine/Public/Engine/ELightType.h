#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

enum class ELightType : Uint8 {
    Ambient,
    Directional,
    Point,
    Spot
};

ReflectEnum(ELightType, Ambient, Directional, Point, Spot);
