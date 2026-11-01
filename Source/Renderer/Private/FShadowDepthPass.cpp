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
        Renderer::FMaterialPipeline* Pipeline{Primitive.mProxy.mSkeletalMesh != nullptr ? mRenderer->GetDepthPipeline(true) : mPipeline};

        if (Pipeline == nullptr) {
            return;
        }

        for (const Asset::FSubMesh& SubMesh : Mesh.GetMeshData().GetSubMeshes()) {
            Commands.emplace_back(Pipeline, nullptr, &Mesh, &Primitive, SubMesh.GetFirstIndex(), SubMesh.GetIndexCount());
        }
    }

}
