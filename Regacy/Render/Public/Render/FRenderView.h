#pragma once

#include <bitset>
#include "RenderCore/FRenderData.h"
#include "Asset/Pipeline/UPipeline.h"
#include "Render/FSceneRenderSurface.h"
#include "Render/ERenderPass.h"

class FRenderScene;
struct FVisibleMeshDrawCommand;

struct FRenderView {
    bool IsPassEnabled(ERenderPass Pass) const;
    void SetPassEnabled(ERenderPass Pass, bool Enabled);
    void CollectMeshDrawCommands(const FRenderScene& Scene, TArray<FVisibleMeshDrawCommand>& OutCommands) const;

    FSceneRenderSurface* mTarget{nullptr};
    FViewMatrices mCamera{};

    FRenderSettings mSettings{};
    ERenderMode mRenderMode{ERenderMode::Lit};
    bool mUseLOD{true};

    std::bitset<static_cast<std::size_t>(ERenderPass::Count)> mPasses{(1ull << static_cast<std::size_t>(ERenderPass::Count)) - 1};
};
