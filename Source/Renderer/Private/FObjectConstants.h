#pragma once

#include "Core/Math/FMatrix.h"

struct FObjectConstants {
    FMatrix mWorld{};
    FMatrix mWorldInverse{};

    std::array<float, 4> mColor{};
};
