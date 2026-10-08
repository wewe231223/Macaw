#pragma once

#include "RenderCore/FSceneHandle.h"
#include "RenderCore/FRenderProbe.h"
#include "RenderCore/FPrimitiveSceneProxy.h"

#include <memory>

enum class EPrimitiveSceneUpdate : Uint8 {
    Create,
    Transform,
    Remove
};

struct FPrimitiveSceneUpdate {
    FObjectHandle mComponentHandle{};
    EPrimitiveSceneUpdate mType{EPrimitiveSceneUpdate::Remove};
    std::unique_ptr<FPrimitiveSceneProxy> mProxy{};
    FPrimitiveTransform mTransform{};
};

struct FSceneUpdateBatch {
    FSceneHandle mSceneHandle{};
    FSceneRenderData mRenderData{};
    bool mFullSnapshot{};
    TArray<FPrimitiveSceneUpdate> mPrimitiveUpdates{};
};
