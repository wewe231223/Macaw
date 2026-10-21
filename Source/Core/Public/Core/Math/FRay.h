#pragma once

#include "Core/Math/FVector.h"

struct FRay {
    FVector mOrigin{};
    FVector mDirection{1.0f, 0.0f, 0.0f};
};
