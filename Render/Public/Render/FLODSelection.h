#pragma once
#include "Asset/FLODSettings.h"

struct FLODSelection {
    Uint32 mLevel{};
    Uint32 mNextLevel{UINT32_MAX};
    // 0: fully visible. Positive: outgoing pixels, negative: complementary incoming pixels.
    float mDither{};
    bool mCulled{};
};

FLODSelection SelectMeshLOD(float ScreenSize, float ViewportHeight, Uint32 AvailableMask, bool AllowCull);
