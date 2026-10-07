#pragma once

#include "RenderCore/FSceneInterface.h"
#include "Render/FRenderScene.h"

#include <memory>

class FRenderer;

class FRendererScene final : public FSceneInterface {
public:
    FRendererScene(FRenderer& Renderer, FSceneHandle Handle);
    ~FRendererScene() override = default;
    FRendererScene(const FRendererScene&) = delete;
    FRendererScene& operator=(const FRendererScene&) = delete;
    FRendererScene(FRendererScene&&) = delete;
    FRendererScene& operator=(FRendererScene&&) = delete;

public:
    FSceneHandle GetHandle() const override;
    bool ApplyUpdates(FSceneUpdateBatch& Updates) override;
    void Release() override;

    FRenderScene& GetRenderScene();
    const FRenderScene& GetRenderScene() const;
    void Reset(Uint64 Generation);
    void ResetRenderData();
    void Detach();

private:
    FRenderer* mRenderer{};
    FSceneHandle mHandle{};
    std::unique_ptr<FRenderScene> mRenderScene{};
};
