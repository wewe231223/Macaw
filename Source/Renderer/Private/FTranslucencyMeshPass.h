#pragma once

#include "FMeshPass.h"

namespace Renderer {

    class FSpriteRenderer;
    class FTextRenderer;

    class FTranslucencyMeshPass final : public Renderer::FMeshPass {
    public:
        struct FRenderParameters {
        public:
            Renderer::FSpriteRenderer* mSpriteRenderer{};
            Renderer::FTextRenderer* mTextRenderer{};
            ID3D11BlendState* mBlend{};
            ID3D11DepthStencilState* mDepthReadOnly{};
            ID3D11DepthStencilState* mDepthDisabled{};
            ID3D11RasterizerState* mRasterizer{};
            ID3D11RasterizerState* mWireframeRasterizer{};
        };

        using Renderer::FMeshPass::FMeshPass;

    public:
        Renderer::EMeshPass GetType() const override;

        Core::Uint32 Render(const Engine::FSceneViewFamily& Family, bool EditorIcons, const Renderer::FTranslucencyMeshPass::FRenderParameters& Parameters);
        Core::Uint32 RenderOverlayTexts(const Engine::FSceneViewFamily& Family, const Renderer::FTranslucencyMeshPass::FRenderParameters& Parameters);

    private:
        bool BeginCommands(const Engine::FSceneViewFamily& Family) override;
        bool Includes(const Renderer::FScenePrimitive& Primitive, const Engine::FSceneViewFamily& Family) const override;

        void AddCommands(const Renderer::FScenePrimitive& Primitive, Asset::UStaticMesh& Mesh, const Engine::FSceneViewFamily& Family, Core::TArray<Renderer::FMeshDrawCommand>& Commands) override;
    };

}
