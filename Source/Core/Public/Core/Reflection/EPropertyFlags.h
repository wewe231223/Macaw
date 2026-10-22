#pragma once

#include "Core/Types.h"

enum class EPropertyFlags : Uint32 {
    None = 0,
    Edit = 1,
    Serialize = 2,
    ReadOnly = 4,
    Transient = 8
};

EPropertyFlags operator|(EPropertyFlags Left, EPropertyFlags Right);
