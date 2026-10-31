#include "FShadowDepthPass.h"

#include "FScenePrimitive.h"
#include "Renderer/Renderer.h"
#include "Asset/UStaticMesh.h"

namespace Renderer {

    Renderer::EMeshPass Renderer::FShadowDepthPass::GetType() const {
        return Renderer::EMeshPass::ShadowDepth;
    }

    bool Renderer::FShadowDepthPass::BeginCommands(const Engine::FSceneViewFamily&) {
        mPipeline = mRenderer->GetDepthPipeline();

        return mPipeline != nullptr;
    }

    bool Renderer::FShadowDepthPass::Includes(const Renderer::FScenePrimitive& Primitive, const Engine::FSceneViewFamily&) const {
        return Primitive.mProxy.mCastShadow;
    }

    void Renderer::FShadowDepthPass::AddCommands(const Renderer::FScenePrimitive& Primitive, Asset::UStaticMesh& Mesh, const Engine::FSceneViewFamily&, Core::TArray<Renderer::FMeshDrawCommand>& Commands) {
        for (const Asset::FSubMesh& SubMesh : Mesh.GetMeshData().GetSubMeshes()) {
            Commands.emplace_back(mPipeline, nullptr, &Mesh, &Primitive, SubMesh.GetFirstIndex(), SubMesh.GetIndexCount());
        }
    }

}
