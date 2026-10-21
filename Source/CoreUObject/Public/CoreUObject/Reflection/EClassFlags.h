#pragma once

#include "Core/Types.h"

enum class EClassFlags : Uint32 {
    None = 0,
    Abstract = 1,
    NoFactory = 2,
    Metadata = 4
};

EClassFlags operator|(EClassFlags Left, EClassFlags Right);
