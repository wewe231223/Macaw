#pragma once

#include "RenderCore/FSceneHandle.h"
#include "RenderCore/FRenderProbe.h"

struct FSceneUpdateBatch {
    FSceneHandle mSceneHandle{};
    FSceneRenderData mRenderData{};
    bool mFullSnapshot{};
};
