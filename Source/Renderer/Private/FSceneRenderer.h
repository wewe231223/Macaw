#pragma once

#include "FSceneDrawContext.h"
#include "RenderCore/FRenderStats.h"

struct FSceneViewFamily;
class FScene;
struct FBox;

class FSceneRenderer {
public:
    FSceneRenderer(const FSceneViewFamily& Family, const FScene& Scene, const FSceneDrawContext& Context, FRenderStats& Stats);

public:
    void Render();

private:
    bool IsVisible(const FBox& Bounds) const;

private:
    const FSceneViewFamily* mFamily{};
    const FScene* mScene{};
    FSceneDrawContext mDrawContext{};
    FRenderStats* mStats{};
};
