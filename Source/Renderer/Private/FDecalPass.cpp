#include "FDecalPass.h"

#include "Core/Math/FBox.h"
#include "Engine/FDecalSceneProxy.h"
#include "Engine/FSceneView.h"
#include "FMeshPass.h"
#include "RenderTarget.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Renderer {

    bool Renderer::FDecalPass::Initialize(ID3D11Device* Device) {
        D3D11_DEPTH_STENCIL_DESC Test{};
        Test.DepthFunc = D3D11_COMPARISON_ALWAYS;
        Test.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        Test.StencilEnable = true;
        Test.StencilReadMask = Renderer::GBuffer::ReceivesDecals;
        Test.FrontFace = {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_EQUAL};
        Test.BackFace = Test.FrontFace;

        D3D11_RASTERIZER_DESC Rasterizer{};
        Rasterizer.FillMode = D3D11_FILL_SOLID;
        Rasterizer.CullMode = D3D11_CULL_NONE;
        Rasterizer.DepthClipEnable = true;
        Rasterizer.ScissorEnable = true;

        D3D11_SAMPLER_DESC Sampler{};
        Sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        Sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        Sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        Sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        Sampler.MaxLOD = D3D11_FLOAT32_MAX;

        Core::FString Error{};

        return mConstants.Create(Device, Error) && SUCCEEDED(Device->CreateDepthStencilState(&Test, mTestReceiver.GetAddressOf())) && SUCCEEDED(Device->CreateRasterizerState(&Rasterizer, mRasterizer.GetAddressOf())) && SUCCEEDED(Device->CreateSamplerState(&Sampler, mSampler.GetAddressOf()));
    }

    bool Renderer::FDecalPass::CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> VertexCode, std::span<const Core::Uint8> PixelCode) {
        Microsoft::WRL::ComPtr<ID3D11VertexShader> Vertex{};
        Microsoft::WRL::ComPtr<ID3D11PixelShader> Pixel{};

        if (FAILED(Device->CreateVertexShader(VertexCode.data(), VertexCode.size(), nullptr, Vertex.GetAddressOf())) || FAILED(Device->CreatePixelShader(PixelCode.data(), PixelCode.size(), nullptr, Pixel.GetAddressOf()))) {
            return false;
        }

        mVertexShader = std::move(Vertex);
        mPixelShader = std::move(Pixel);

        return true;
    }

    Core::Uint32 Renderer::FDecalPass::Render(ID3D11Device* Device, ID3D11DeviceContext* Context, const Renderer::FDXRenderTarget& Target, const Engine::FSceneView& View, std::span<const Engine::FDecalSceneProxy> Decals, ID3D11BlendState* AlphaBlend) {
        FConstants Constants{};

        if (!View.mViewProjection.TryInverse(Constants.mInverseViewProjection)) {
            return 0;
        }

        const Core::Uint32 Width{Target.GetWidth()};
        const Core::Uint32 Height{Target.GetHeight()};
        Constants.mScreenAndOpacity = {1.0f / static_cast<float>(Width), 1.0f / static_cast<float>(Height), 0.0f, 0.0f};

        ID3D11RenderTargetView* Color{Target.GetGBuffer().front()->GetRenderTargetView()};
        ID3D11ShaderResourceView* Depth{Target.GetSceneDepth()->GetShaderResourceView()};
        ID3D11SamplerState* Sampler{mSampler.Get()};

        Context->OMSetRenderTargets(1, &Color, Target.GetReadOnlyDepthView());
        Context->OMSetDepthStencilState(mTestReceiver.Get(), Renderer::GBuffer::ReceivesDecals);
        Context->OMSetBlendState(AlphaBlend, nullptr, 0xFFFFFFFFu);
        Context->RSSetState(mRasterizer.Get());
        Context->IASetInputLayout(nullptr);
        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Context->VSSetShader(mVertexShader.Get(), nullptr, 0);
        Context->PSSetShader(mPixelShader.Get(), nullptr, 0);
        Context->PSSetSamplers(0, 1, &Sampler);
        Context->PSSetShaderResources(1, 1, &Depth);
        mConstants.Bind(Context, RenderCore::EShaderStage::Pixel, 0);

        Core::Uint32 DrawCalls{};

        for (const Engine::FDecalSceneProxy& Decal : Decals) {
            const D3D11_RECT Scissor{GetScissor(Decal.mDecalToWorld * View.mViewProjection, Width, Height)};

            if (Scissor.left >= Scissor.right || Scissor.top >= Scissor.bottom || (!Decal.mTexture->HasResources() && !Decal.mTexture->CreateResources(Device))) {
                continue;
            }

            Constants.mWorldToDecal = Decal.mWorldToDecal;
            Constants.mScreenAndOpacity.z = Decal.mOpacity;
            mConstants.Update(Context, Constants);

            ID3D11ShaderResourceView* Texture{Decal.mTexture->GetShaderResourceView()};
            Context->RSSetScissorRects(1, &Scissor);
            Context->PSSetShaderResources(0, 1, &Texture);
            Context->Draw(3, 0);
            ++DrawCalls;
        }

        const std::array<ID3D11ShaderResourceView*, 2> Empty{};
        Context->PSSetShaderResources(0, static_cast<UINT>(Empty.size()), Empty.data());
        Context->OMSetRenderTargets(0, nullptr, nullptr);

        return DrawCalls;
    }

    D3D11_RECT Renderer::FDecalPass::GetScissor(const Core::FMatrix& DecalToClip, Core::Uint32 Width, Core::Uint32 Height) {
        const float ScreenWidth{static_cast<float>(Width)};
        const float ScreenHeight{static_cast<float>(Height)};
        float Left{ScreenWidth};
        float Top{ScreenHeight};
        float Right{};
        float Bottom{};

        for (Core::Uint32 Corner{}; Corner < 8; ++Corner) {
            const Core::FVector4 Local{(Corner & 1u) != 0 ? 1.0f : -1.0f, (Corner & 2u) != 0 ? 1.0f : -1.0f, (Corner & 4u) != 0 ? 1.0f : -1.0f, 1.0f};
            const Core::FVector4 Clip{Local * DecalToClip};

            if (Clip.z < 0.0f || Clip.w <= 0.0f) {
                const Core::FBox UnitBox{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};

                return Renderer::FMeshPass::IsVisible(DecalToClip, UnitBox) ? D3D11_RECT{0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height)} : D3D11_RECT{};
            }

            const float X{std::clamp(0.5f + 0.5f * Clip.x / Clip.w, 0.0f, 1.0f) * ScreenWidth};
            const float Y{std::clamp(0.5f - 0.5f * Clip.y / Clip.w, 0.0f, 1.0f) * ScreenHeight};
            Left = std::min(Left, X);
            Top = std::min(Top, Y);
            Right = std::max(Right, X);
            Bottom = std::max(Bottom, Y);
        }

        return {static_cast<LONG>(std::floor(Left)), static_cast<LONG>(std::floor(Top)), static_cast<LONG>(std::ceil(Right)), static_cast<LONG>(std::ceil(Bottom))};
    }

}
