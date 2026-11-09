/**
 * @file FShadowDepthPass.cpp
 * @brief 그림자를 드리우는 메시를 깊이 전용 파이프라인으로 아틀라스 타일에 기록하는 메시 패스입니다.
 * @ingroup Renderer
 */

#include "FShadowDepthPass.h"

#include "FScenePrimitive.h"
#include "Renderer/Renderer.h"
#include "Asset/UStaticMesh.h"

#include <utility>

namespace Renderer {

    bool Renderer::FShadowDepthPass::Initialize() {
        decltype(mRasterizers) Rasterizers{};
        D3D11_RASTERIZER_DESC Description{};
        Description.FillMode = D3D11_FILL_SOLID;
        Description.CullMode = D3D11_CULL_NONE;
        Description.DepthClipEnable = true;
        Description.ScissorEnable = true;
        Description.DepthBiasClamp = 0.005f;

        for (std::size_t DepthLevel{}; DepthLevel < Rasterizers.size(); ++DepthLevel) {
            Description.DepthBias = static_cast<INT>(DepthLevel) * Engine::ShadowDepthBiasStep;
            auto& SlopeRasterizers{Rasterizers[DepthLevel]};

            for (std::size_t SlopeLevel{}; SlopeLevel < SlopeRasterizers.size(); ++SlopeLevel) {
                Description.SlopeScaledDepthBias = static_cast<float>(SlopeLevel) * Engine::ShadowSlopeBiasStep;

                if (FAILED(mDevice->CreateRasterizerState(&Description, SlopeRasterizers[SlopeLevel].GetAddressOf()))) {
                    return false;
                }
            }
        }

        mRasterizers = std::move(Rasterizers);

        return true;
    }

    ID3D11RasterizerState* Renderer::FShadowDepthPass::GetRasterizer(Engine::EShadowBiasPreset DepthPreset, Engine::EShadowBiasPreset SlopePreset) const {
        return mRasterizers[static_cast<Core::Uint32>(DepthPreset)][static_cast<Core::Uint32>(SlopePreset)].Get();
    }

    /**
     * @brief 깊이 전용 입력 레이아웃을 쓰는 그림자 깊이 패스 종류를 반환합니다.
     */
    Renderer::EMeshPass Renderer::FShadowDepthPass::GetType() const {
        return Renderer::EMeshPass::ShadowDepth;
    }

    /**
     * @brief 모든 드로가 공유할 깊이 전용 파이프라인을 조회합니다.
     */
    bool Renderer::FShadowDepthPass::BeginCommands(const Engine::FSceneViewFamily&) {
        mPipeline = mRenderer->GetDepthPipeline();

        return mPipeline != nullptr;
    }

    /**
     * @brief 그림자를 드리우는 프리미티브만 대상으로 합니다.
     */
    bool Renderer::FShadowDepthPass::Includes(const Renderer::FScenePrimitive& Primitive, const Engine::FSceneViewFamily&) const {
        return Primitive.mProxy.mCastShadow;
    }

    /**
     * @brief SubMesh마다 머티리얼 없이 깊이 전용 파이프라인 드로를 추가합니다.
     */
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
