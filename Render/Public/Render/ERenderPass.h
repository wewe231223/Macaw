#pragma once
#include "Core/Common.h"

enum class ERenderPass : Uint8 {
    Opaque,
    Translucent,
    PostProcessing,
    Text,
    Billboard,
    Count
};
