#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "EBlendMode.generated.h"

namespace Asset {

    UENUM()
    enum class EBlendMode : Core::Uint8 {
        Opaque,
        Translucent
    };

}
