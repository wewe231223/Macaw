#pragma once

#include "Core/Common.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "RenderCore/FMaterialBlendMode.generated.h"

UENUM()
enum class EMaterialBlendMode : Uint8 {
    Opaque,
    Translucent
};
