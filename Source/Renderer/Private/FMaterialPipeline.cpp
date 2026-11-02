/**
 * @file FMaterialPipeline.cpp
 * @brief 머티리얼 셰이더와 양면 정책별 GPU 파이프라인을 관리합니다.
 * @ingroup Renderer
 */

#include "FMaterialPipeline.h"

#include "FShaderInstance.h"

#include <utility>

namespace Renderer {

    Renderer::FMaterialPipeline::FMaterialPipeline(Renderer::EMeshPass Pass, std::shared_ptr<Renderer::FShaderInstance> VertexShader, std::shared_ptr<Renderer::FShaderInstance> PixelShader, bool TwoSided, bool Instanced, bool Skinned)
		: mPass{Pass},
		  mSkinned{Skinned},
		  mTwoSided{TwoSided},
		  mInstanced{Instanced && !Skinned},
		  mVertexShader{std::move(VertexShader)},
		  mPixelShader{std::move(PixelShader)} {
    }

    Renderer::FMaterialPipeline::~FMaterialPipeline() = default;

    bool Renderer::FMaterialPipeline::IsCurrent() const {
        return HasResources() && mVertexShader->IsCurrent() && (mPixelShader == nullptr || mPixelShader->IsCurrent());
    }

    bool Renderer::FMaterialPipeline::HasResources() const {
        return mVertexShader != nullptr && mVertexShader->HasResources() && (mPixelShader == nullptr || mPixelShader->HasResources()) && mRasterizer != nullptr;
    }

    bool Renderer::FMaterialPipeline::IsInstanced() const {
        return mInstanced;
    }

    bool Renderer::FMaterialPipeline::IsSkinned() const {
        return mSkinned;
    }

    Core::Uint64 Renderer::FMaterialPipeline::GetVertexRevision() const {
        return mVertexShader != nullptr ? mVertexShader->GetRevision() : 0;
    }

    bool Renderer::FMaterialPipeline::CreateResources(ID3D11Device* Device, Core::FString& Error) {
        Error.clear();

        if (Device == nullptr || mVertexShader == nullptr || (mPass != Renderer::EMeshPass::ShadowDepth && mPixelShader == nullptr)) {
            Error = "Invalid material shader";
            return false;
        }

        if (!mVertexShader->CreateResources(Device, Error) || (mPixelShader != nullptr && !mPixelShader->CreateResources(Device, Error))) {
            return false;
        }

        if (mRasterizer != nullptr) {
            return true;
        }

        D3D11_RASTERIZER_DESC Description{};
        Description.FillMode = D3D11_FILL_SOLID;
        Description.CullMode = mTwoSided ? D3D11_CULL_NONE : D3D11_CULL_BACK;
        Description.DepthClipEnable = true;
        Description.ScissorEnable = mPass == Renderer::EMeshPass::ShadowDepth;

        if (FAILED(Device->CreateRasterizerState(&Description, mRasterizer.GetAddressOf()))) {
            Error = "Cannot create the material rasterizer";
            return false;
        }

        return true;
    }

    void Renderer::FMaterialPipeline::Bind(ID3D11DeviceContext* Context) const {
        mVertexShader->Bind(Context);

        if (mPixelShader != nullptr) {
            mPixelShader->Bind(Context);
        } else {
            Context->PSSetShader(nullptr, nullptr, 0);
        }

        Context->RSSetState(mRasterizer.Get());
    }

}
