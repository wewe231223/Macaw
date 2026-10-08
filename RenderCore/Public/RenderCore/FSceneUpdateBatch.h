#pragma once
#include "RenderCore/FSceneHandle.h"
#include "RenderCore/FPrimitiveSceneProxy.h"
#include "RenderCore/FLightSceneProxy.h"

#include <memory>

enum class ESceneUpdateType : Uint8 {
    Create,
    Transform,
    Remove
};

struct FPrimitiveSceneUpdate {
    FObjectHandle mComponentHandle{};
    ESceneUpdateType mType{ESceneUpdateType::Remove};
    std::unique_ptr<FPrimitiveSceneProxy> mProxy{};
    FPrimitiveTransform mTransform{};
};

struct FLightSceneUpdate {
    FObjectHandle mComponentHandle{};
    ESceneUpdateType mType{ESceneUpdateType::Remove};
    std::unique_ptr<FLightSceneProxy> mProxy{};
    FMatrix mWorld{};
};

struct FSceneUpdateBatch {
    FSceneHandle mSceneHandle{};
    bool mFullSnapshot{};
    TArray<FPrimitiveSceneUpdate> mPrimitiveUpdates{};
    TArray<FLightSceneUpdate> mLightUpdates{};
};
