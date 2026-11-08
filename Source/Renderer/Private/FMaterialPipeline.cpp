/**
 * @file FMaterialPipeline.cpp
 * @brief 머티리얼 셰이더와 양면 정책별 GPU 파이프라인을 관리합니다.
 * @ingroup Renderer
 */

#include "FMaterialPipeline.h"

#include "FShaderInstance.h"

#include <utility>

namespace Renderer {

    /**
     * @brief 전달된 설정과 기본값으로 FMaterialPipeline 객체를 생성합니다.
     */
    Renderer::FMaterialPipeline::FMaterialPipeline(Renderer::EMeshPass Pass, std::shared_ptr<Renderer::FShaderInstance> VertexShader, std::shared_ptr<Renderer::FShaderInstance> PixelShader, bool TwoSided, bool Instanced, bool Skinned, bool WritesGBuffer)
		: mPass{Pass},
		  mSkinned{Skinned},
		  mTwoSided{TwoSided},
		  mInstanced{Instanced && !Skinned},
		  mWritesGBuffer{WritesGBuffer},
		  mVertexShader{std::move(VertexShader)},
		  mPixelShader{std::move(PixelShader)} {
    }

    /**
     * @brief 기본 소멸자이며, GPU 자원은 ComPtr 멤버가 해제합니다.
     */
    Renderer::FMaterialPipeline::~FMaterialPipeline() = default;

    /**
     * @brief GPU 파이프라인이 현재 셰이더 개정과 일치하는지 검사합니다.
     */
    bool Renderer::FMaterialPipeline::IsCurrent() const {
        return HasResources() && mVertexShader->IsCurrent() && (mPixelShader == nullptr || mPixelShader->IsCurrent());
    }

    /**
     * @brief 렌더링에 필요한 GPU 자원이 준비되었는지 검사합니다.
     */
    bool Renderer::FMaterialPipeline::HasResources() const {
        return mVertexShader != nullptr && mVertexShader->HasResources() && (mPixelShader == nullptr || mPixelShader->HasResources()) && mRasterizer != nullptr;
    }

    /**
     * @brief 메시 인스턴스 데이터를 사용하는 파이프라인인지 반환합니다.
     */
    bool Renderer::FMaterialPipeline::IsInstanced() const {
        return mInstanced;
    }

    /**
     * @brief 객체별 본 행렬을 사용하는지 반환합니다.
     */
    bool Renderer::FMaterialPipeline::IsSkinned() const {
        return mSkinned;
    }

    bool Renderer::FMaterialPipeline::WritesGBuffer() const {
        return mWritesGBuffer;
    }

    /**
     * @brief GPU 파이프라인을 만들 때 사용한 정점 셰이더의 리비전을 반환합니다.
     */
    Core::Uint64 Renderer::FMaterialPipeline::GetVertexRevision() const {
        return mVertexShader != nullptr ? mVertexShader->GetRevision() : 0;
    }

    /**
     * @brief 필요한 GPU 자원을 생성하거나 변경된 상수를 업로드합니다.
     */
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

    /**
     * @brief 드로에 사용할 셰이더·입력 레이아웃·래스터 상태를 바인딩합니다.
     */
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
