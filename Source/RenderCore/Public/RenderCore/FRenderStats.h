#pragma once

#include "Core/Types.h"

struct FRenderStats {
    Uint32 mDrawCalls{};
    Uint32 mVisiblePrimitives{};
    Uint32 mTransformUpdates{};

    std::size_t mGpuBytes{};
};
