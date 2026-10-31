#include "FBasePass.h"

#include "FScenePrimitive.h"
#include "Renderer/Renderer.h"
#include "Asset/UMaterial.h"
#include "Asset/UStaticMesh.h"
#include "Engine/FSceneViewFamily.h"

namespace Renderer {

    Renderer::EMeshPass Renderer::FBasePass::GetType() const {
        return Renderer::EMeshPass::BasePass;
    }

    bool Renderer::FBasePass::BeginCommands(const Engine::FSceneViewFamily& Family) {
        mDefaultMaterial = mRenderer->GetDefaultMaterial();
        mDefaultPipeline = mDefaultMaterial != nullptr ? mRenderer->GetMaterialPipeline(*mDefaultMaterial, Family.mViewMode) : nullptr;

        return mDefaultPipeline != nullptr && mDefaultMaterial->CreateResources(mDevice, mContext);
    }

    bool Renderer::FBasePass::Includes(const Renderer::FScenePrimitive&, const Engine::FSceneViewFamily& Family) const {
        return Family.mShowFlags.mPrimitives;
    }

    void Renderer::FBasePass::AddCommands(const Renderer::FScenePrimitive& Primitive, Asset::UStaticMesh& Mesh, const Engine::FSceneViewFamily& Family, Core::TArray<Renderer::FMeshDrawCommand>& Commands) {
        for (const Asset::FSubMesh& SubMesh : Mesh.GetMeshData().GetSubMeshes()) {
            Asset::UMaterial* SlotMaterial{SubMesh.GetMaterialIndex() < Primitive.mProxy.mMaterials.size() ? Primitive.mProxy.mMaterials[SubMesh.GetMaterialIndex()] : nullptr};
            Asset::UMaterial* Material{Primitive.mProxy.mMaterial != nullptr ? Primitive.mProxy.mMaterial : SlotMaterial != nullptr ? SlotMaterial : mDefaultMaterial};

            if (!Material->CreateResources(mDevice, mContext)) {
                Material = mDefaultMaterial;
            }

            Renderer::FMaterialPipeline* Pipeline{mRenderer->GetMaterialPipeline(*Material, Family.mViewMode)};
            Commands.emplace_back(Pipeline != nullptr ? Pipeline : mDefaultPipeline, Material, &Mesh, &Primitive, SubMesh.GetFirstIndex(), SubMesh.GetIndexCount());
        }
    }

}
