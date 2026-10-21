#pragma once

#include "Core/Math/FVector.h"

struct FDrawLine {
    FVector mStart{};
    FVector mEnd{};
    FVector mColor{1.0f, 1.0f, 1.0f};
    bool mDepthTest{};
};
