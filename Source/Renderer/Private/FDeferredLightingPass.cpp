#include "FDeferredLightingPass.h"

#include "Engine/FSceneViewFamily.h"
#include "FPassTarget.h"
#include "RenderTarget.h"

namespace Renderer {

    bool Renderer::FDeferredLightingPass::Initialize(ID3D11Device* Device) {
        Core::FString Error{};

        return mConstants.Create(Device, Error);
    }

    bool Renderer::FDeferredLightingPass::CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> VertexCode, std::span<const Core::Uint8> PixelCode) {
        Microsoft::WRL::ComPtr<ID3D11VertexShader> Vertex{};
        Microsoft::WRL::ComPtr<ID3D11PixelShader> Pixel{};

        if (FAILED(Device->CreateVertexShader(VertexCode.data(), VertexCode.size(), nullptr, Vertex.GetAddressOf())) || FAILED(Device->CreatePixelShader(PixelCode.data(), PixelCode.size(), nullptr, Pixel.GetAddressOf()))) {
            return false;
        }

        mVertexShader = std::move(Vertex);
        mPixelShader = std::move(Pixel);

        return true;
    }

    Core::Uint32 Renderer::FDeferredLightingPass::Render(ID3D11DeviceContext* Context, const Renderer::FDXRenderTarget& Target, const Renderer::FPassTarget& Output, const Engine::FSceneViewFamily& Family, ID3D11RasterizerState* Rasterizer) {
        FConstants Constants{};

        if (!Family.mView.mViewProjection.TryInverse(Constants.mInverseViewProjection)) {
            return 0;
        }

        Constants.mInverseSize = {1.0f / static_cast<float>(Target.GetWidth()), 1.0f / static_cast<float>(Target.GetHeight())};
        Constants.mViewMode = static_cast<Core::Uint32>(Family.mViewMode);
        mConstants.Update(Context, Constants);
        mConstants.Bind(Context, RenderCore::EShaderStage::Pixel, 0);

        constexpr UINT ResourceSlot{11};
        std::array<ID3D11ShaderResourceView*, Renderer::GBuffer::AttachmentCount + 2> Resources{};

        for (std::size_t Index{}; Index < Target.GetGBuffer().size(); ++Index) {
            Resources[Index] = Target.GetGBuffer()[Index]->GetShaderResourceView();
        }

        Resources[Renderer::GBuffer::AttachmentCount] = Target.GetSceneDepth()->GetShaderResourceView();
        Resources[Renderer::GBuffer::AttachmentCount + 1] = Target.GetStencilResource();

        Context->OMSetRenderTargets(1, &Output.mColor, nullptr);
        Context->OMSetDepthStencilState(nullptr, 0);
        Context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
        Context->RSSetViewports(1, &Output.mViewport);
        Context->RSSetState(Rasterizer);
        Context->IASetInputLayout(nullptr);
        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Context->VSSetShader(mVertexShader.Get(), nullptr, 0);
        Context->PSSetShader(mPixelShader.Get(), nullptr, 0);
        Context->PSSetShaderResources(ResourceSlot, static_cast<UINT>(Resources.size()), Resources.data());
        Context->Draw(3, 0);

        Resources.fill(nullptr);
        Context->PSSetShaderResources(ResourceSlot, static_cast<UINT>(Resources.size()), Resources.data());

        return 1;
    }

}
