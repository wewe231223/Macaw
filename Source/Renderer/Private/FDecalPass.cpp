/**
 * @file FDecalPass.cpp
 * @brief 깊이로 복원한 표면에 데칼을 투영하고 GBuffer의 색상·노멀을 갱신합니다.
 * @ingroup Renderer
 */

#include "FDecalPass.h"

#include "Core/Math/FBox.h"
#include "Engine/FDecalSceneProxy.h"
#include "Engine/FSceneViewFamily.h"
#include "FMeshPass.h"
#include "RenderTarget.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Renderer {

    bool Renderer::FDecalPass::Initialize(ID3D11Device* Device, const D3D11_BLEND_DESC& ColorBlend) {
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

        D3D11_BLEND_DESC Blend{ColorBlend};
        Blend.IndependentBlendEnable = true;
        Blend.RenderTarget[1] = Blend.RenderTarget[0];
        Blend.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;

        Core::FString Error{};

        return mConstants.Create(Device, Error) && SUCCEEDED(Device->CreateDepthStencilState(&Test, mTestReceiver.GetAddressOf())) && SUCCEEDED(Device->CreateRasterizerState(&Rasterizer, mRasterizer.GetAddressOf())) && SUCCEEDED(Device->CreateSamplerState(&Sampler, mSampler.GetAddressOf())) && SUCCEEDED(Device->CreateBlendState(&Blend, mBlend.GetAddressOf()));
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

    Core::Uint32 Renderer::FDecalPass::Render(ID3D11Device* Device, ID3D11DeviceContext* Context, Renderer::FDXRenderTarget& Target, const Engine::FSceneViewFamily& Family, std::span<const Engine::FDecalSceneProxy> Decals) {
        const Engine::FSceneView& View{Family.mView};
        FConstants Constants{};

        if (!View.mViewProjection.TryInverse(Constants.mInverseViewProjection)) {
            return 0;
        }

        const Core::Uint32 Width{Target.GetWidth()};
        const Core::Uint32 Height{Target.GetHeight()};
        constexpr UINT NormalMapSlot{11};
        constexpr UINT HistorySlot{14};
        Constants.mScreenAndOpacity = {1.0f / static_cast<float>(Width), 1.0f / static_cast<float>(Height), 0.0f, 0.0f};

        Asset::UTexture2D* SurfaceNormals{Target.GetGBuffer()[Renderer::GBuffer::NormalAttachment].get()};
        const std::array<ID3D11RenderTargetView*, 2> Outputs{Target.GetGBuffer().front()->GetRenderTargetView(), SurfaceNormals->GetRenderTargetView()};
        ID3D11ShaderResourceView* Depth{Target.GetSceneDepth()->GetShaderResourceView()};
        ID3D11SamplerState* Sampler{mSampler.Get()};

        Context->OMSetRenderTargets(static_cast<UINT>(Outputs.size()), Outputs.data(), Target.GetReadOnlyDepthView());
        Context->OMSetDepthStencilState(mTestReceiver.Get(), Renderer::GBuffer::ReceivesDecals);
        Context->OMSetBlendState(mBlend.Get(), nullptr, 0xFFFFFFFFu);
        Context->RSSetState(mRasterizer.Get());
        Context->IASetInputLayout(nullptr);
        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Context->VSSetShader(mVertexShader.Get(), nullptr, 0);
        Context->PSSetShader(mPixelShader.Get(), nullptr, 0);
        Context->PSSetSamplers(0, 1, &Sampler);
        mConstants.Bind(Context, RenderCore::EShaderStage::Pixel, 0);

        const bool AllowColor{Family.mViewMode != Engine::EViewMode::WorldNormal};
        bool AllowNormals{Family.mViewMode == Engine::EViewMode::Lit_Phong || Family.mViewMode == Engine::EViewMode::Lit_Lambert || Family.mViewMode == Engine::EViewMode::WorldNormal};
        Asset::UTexture2D* ReceiverSource{};
        D3D11_RECT WrittenNormals{};
        Core::Uint32 DrawCalls{};

        for (const Engine::FDecalSceneProxy& Decal : Decals) {
            const D3D11_RECT Scissor{GetScissor(Decal.mDecalToWorld * View.mViewProjection, Width, Height)};

            if (Scissor.left >= Scissor.right || Scissor.top >= Scissor.bottom) {
                continue;
            }

            Asset::UTexture2D* Color{Decal.mTexture};
            Asset::UTexture2D* Normal{AllowNormals && Decal.mNormalStrength > 0.0f ? Decal.mNormalMap : nullptr};
            const bool DrawColor{AllowColor && Color != nullptr};

            if ((!DrawColor && Normal == nullptr) || (Color != nullptr && !Color->HasResources() && !Color->CreateResources(Device))) {
                continue;
            }

            if (Normal != nullptr && !Normal->HasResources() && !Normal->CreateResources(Device)) {
                Normal = nullptr;
            }

            if (Normal != nullptr && ReceiverSource == nullptr) {
                ReceiverSource = Target.GetDecalNormalSource(Device, false);

                if (ReceiverSource != nullptr) {
                    Context->OMSetRenderTargets(1, Outputs.data(), Target.GetReadOnlyDepthView());
                    Context->CopyResource(ReceiverSource->GetTexture(), SurfaceNormals->GetTexture());
                    Context->OMSetRenderTargets(static_cast<UINT>(Outputs.size()), Outputs.data(), Target.GetReadOnlyDepthView());

                    const std::array<ID3D11ShaderResourceView*, 2> Receivers{ReceiverSource->GetShaderResourceView(), Target.GetStencilResource()};
                    Context->PSSetShaderResources(NormalMapSlot + 1, static_cast<UINT>(Receivers.size()), Receivers.data());
                } else {
                    AllowNormals = false;
                    Normal = nullptr;
                }
            }

            Asset::UTexture2D* PreviousNormals{ReceiverSource};
            D3D11_RECT Overlap{};

            if (Normal != nullptr && IntersectRect(&Overlap, &WrittenNormals, &Scissor)) {
                PreviousNormals = Target.GetDecalNormalSource(Device, true);

                if (PreviousNormals != nullptr) {
                    const D3D11_BOX Region{static_cast<UINT>(Scissor.left), static_cast<UINT>(Scissor.top), 0, static_cast<UINT>(Scissor.right), static_cast<UINT>(Scissor.bottom), 1};
                    ID3D11ShaderResourceView* Empty{};
                    Context->PSSetShaderResources(HistorySlot, 1, &Empty);
                    Context->OMSetRenderTargets(1, Outputs.data(), Target.GetReadOnlyDepthView());
                    Context->CopySubresourceRegion(PreviousNormals->GetTexture(), 0, Region.left, Region.top, 0, SurfaceNormals->GetTexture(), 0, &Region);
                    Context->OMSetRenderTargets(static_cast<UINT>(Outputs.size()), Outputs.data(), Target.GetReadOnlyDepthView());
                } else {
                    Normal = nullptr;
                }
            }

            if (!DrawColor && Normal == nullptr) {
                continue;
            }

            Constants.mWorldToDecal = Decal.mWorldToDecal;
            Constants.mScreenAndOpacity.z = Decal.mOpacity;
            Constants.mScreenAndOpacity.w = Normal != nullptr ? Decal.mNormalStrength : 0.0f;
            Constants.mTextureFlags = {Color != nullptr, DrawColor, 0u, 0u};
            mConstants.Update(Context, Constants);

            const std::array<ID3D11ShaderResourceView*, 2> Textures{Color != nullptr ? Color->GetShaderResourceView() : nullptr, Depth};
            Context->RSSetScissorRects(1, &Scissor);
            Context->PSSetShaderResources(0, static_cast<UINT>(Textures.size()), Textures.data());

            if (ReceiverSource != nullptr) {
                ID3D11ShaderResourceView* NormalMap{Normal != nullptr ? Normal->GetShaderResourceView() : nullptr};
                ID3D11ShaderResourceView* History{Normal != nullptr ? PreviousNormals->GetShaderResourceView() : nullptr};
                Context->PSSetShaderResources(NormalMapSlot, 1, &NormalMap);
                Context->PSSetShaderResources(HistorySlot, 1, &History);
            }

            if (Normal != nullptr) {
                D3D11_RECT Combined{};
                UnionRect(&Combined, &WrittenNormals, &Scissor);
                WrittenNormals = Combined;
            }

            Context->Draw(3, 0);
            ++DrawCalls;
        }

        const std::array<ID3D11ShaderResourceView*, 4> Empty{};
        Context->PSSetShaderResources(0, 2, Empty.data());
        Context->PSSetShaderResources(NormalMapSlot, static_cast<UINT>(Empty.size()), Empty.data());
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
