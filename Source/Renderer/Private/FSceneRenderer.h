#pragma once

#include "Core/ReflectionMacros.h"
#include "FSceneDrawContext.h"
#include "RenderCore/FRenderStats.h"

struct FSceneViewFamily;
class FScene;
struct FBox;

class FSceneRenderer {
public:
    ReflectTypes(FSceneRenderer);

public:
    FSceneRenderer(const FSceneViewFamily& Family, const FScene& Scene, const FSceneDrawContext& Context, FRenderStats& Stats);

public:
    ReflectStruct(FSceneRenderer);

    void Render();

private:
    bool IsVisible(const FBox& Bounds) const;

private:
    ReflectMembers();

private:
    const FSceneViewFamily* mFamily{};
    ReflectProperty(mFamily, "Family", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    const FScene* mScene{};
    ReflectProperty(mScene, "Scene", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FSceneDrawContext mDrawContext{};
    ReflectProperty(mDrawContext, "DrawContext", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FRenderStats* mStats{};
    ReflectProperty(mStats, "Stats", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
