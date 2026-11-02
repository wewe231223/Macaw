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
        Renderer::FMaterialPipeline* Pipeline{mDefaultMaterial != nullptr ? mRenderer->GetMaterialPipeline(*mDefaultMaterial, Family.mViewMode, false, Family.mShowFlags.mShadows) : nullptr};

        return Pipeline != nullptr && mDefaultMaterial->CreateResources(mDevice, mContext);
    }

    bool Renderer::FBasePass::Includes(const Renderer::FScenePrimitive&, const Engine::FSceneViewFamily& Family) const {
        return Family.mShowFlags.mPrimitives;
    }

    void Renderer::FBasePass::AddCommands(const Renderer::FScenePrimitive& Primitive, Asset::UStaticMesh& Mesh, const Engine::FSceneViewFamily& Family, Core::TArray<Renderer::FMeshDrawCommand>& Commands) {
        const bool Skinned{Primitive.mProxy.mSkeletalMesh != nullptr};
        const bool ReceiveShadows{Family.mShowFlags.mShadows && Primitive.mProxy.mReceiveShadow};

        for (const Asset::FSubMesh& SubMesh : Mesh.GetMeshData().GetSubMeshes()) {
            Asset::UMaterial* SlotMaterial{SubMesh.GetMaterialIndex() < Primitive.mProxy.mMaterials.size() ? Primitive.mProxy.mMaterials[SubMesh.GetMaterialIndex()] : nullptr};
            Asset::UMaterial* Material{Primitive.mProxy.mMaterial != nullptr ? Primitive.mProxy.mMaterial : SlotMaterial != nullptr ? SlotMaterial : mDefaultMaterial};

            if (!Material->CreateResources(mDevice, mContext)) {
                Material = mDefaultMaterial;
            }

            Renderer::FMaterialPipeline* Pipeline{mRenderer->GetMaterialPipeline(*Material, Family.mViewMode, Skinned, ReceiveShadows)};

            if (Pipeline == nullptr) {
                // 실패한 스킨 셰이더를 정적 파이프라인으로 대신 그리면 바인드 포즈가 노출됩니다.
                Material = mDefaultMaterial;
                Pipeline = mRenderer->GetMaterialPipeline(*Material, Family.mViewMode, Skinned, ReceiveShadows);
            }

            if (Pipeline != nullptr) {
                Commands.emplace_back(Pipeline, Material, &Mesh, &Primitive, SubMesh.GetFirstIndex(), SubMesh.GetIndexCount());
            }
        }
    }

}
