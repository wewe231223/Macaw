#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

enum class EPrimitiveType : Uint8 {
    Plane,
    Cube,
    Sphere,
    Count
};

DeclareReflectEnum(EPrimitiveType);
