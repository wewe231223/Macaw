/**
 * @file FOpaqueMeshPass.cpp
 * @brief 보이는 메시의 표면과 깊이를 G-buffer에 기록하는 메시 패스입니다.
 * @ingroup Renderer
 */

#include "FOpaqueMeshPass.h"
#include "Asset/USkeletalMesh.h"

#include "FScenePrimitive.h"
#include "Renderer/Renderer.h"
#include "Asset/UMaterial.h"
#include "Asset/UStaticMesh.h"
#include "Engine/FSceneViewFamily.h"

namespace Renderer {

    /**
     * @brief 표면과 깊이를 기록하는 기본 패스 종류를 반환합니다.
     */
    Renderer::EMeshPass Renderer::FOpaqueMeshPass::GetType() const {
        return Renderer::EMeshPass::OpaqueMeshPass;
    }

    /**
     * @brief 머티리얼이 없거나 준비에 실패한 SubMesh가 쓸 기본 머티리얼과 파이프라인을 준비합니다.
     */
    bool Renderer::FOpaqueMeshPass::BeginCommands(const Engine::FSceneViewFamily& Family) {
        mDefaultMaterial = mRenderer->GetDefaultMaterial();

        Renderer::FMaterialPipeline* Pipeline{mDefaultMaterial != nullptr ? mRenderer->GetMaterialPipeline(*mDefaultMaterial, Family.mViewMode, false, Family.mShowFlags.mShadows) : nullptr};

        return Pipeline != nullptr && mDefaultMaterial->CreateResources(mDevice, mContext);
    }

    /**
     * @brief 프리미티브 표시 플래그가 켜져 있으면 모든 메시 프리미티브를 대상으로 합니다.
     */
    bool Renderer::FOpaqueMeshPass::Includes(const Renderer::FScenePrimitive&, const Engine::FSceneViewFamily& Family) const {
        return Family.mShowFlags.mPrimitives;
    }

    /**
     * @brief SubMesh마다 덮어쓰기·슬롯·기본 머티리얼 순으로 머티리얼을 정해 해당 파이프라인 드로를 추가합니다.
     */
    void Renderer::FOpaqueMeshPass::AddCommands(const Renderer::FScenePrimitive& Primitive, Asset::UStaticMesh& Mesh, const Engine::FSceneViewFamily& Family, Core::TArray<Renderer::FMeshDrawCommand>& Commands) {
        const bool Skinned{Primitive.mProxy.mSkeletalMesh != nullptr};
        const bool ReceiveShadows{Family.mShowFlags.mShadows && Primitive.mProxy.mReceiveShadow};

        for (const Asset::FSubMesh& SubMesh : Mesh.GetMeshData().GetSubMeshes()) {
            Asset::UMaterial* SlotMaterial{SubMesh.GetMaterialIndex() < Primitive.mProxy.mMaterials.size() ? Primitive.mProxy.mMaterials[SubMesh.GetMaterialIndex()] : nullptr};
            Asset::UMaterial* Material{Primitive.mProxy.mMaterial != nullptr ? Primitive.mProxy.mMaterial.Get() : SlotMaterial != nullptr ? SlotMaterial : mDefaultMaterial.Get()};

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
