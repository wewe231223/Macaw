#pragma once

#include "Core/Math/FMatrix.h"
#include "Core/Math/FVector4.h"
#include "RenderCore/TConstantBuffer.h"

#include <array>
#include <d3d11.h>
#include <span>
#include <wrl/client.h>

namespace Engine {
    struct FDecalSceneProxy;
    struct FSceneViewFamily;
}

namespace Renderer {

    class FDXRenderTarget;

    class FDecalPass {
    private:
        struct FConstants {
            Core::FMatrix mInverseViewProjection{};
            Core::FMatrix mWorldToDecal{};
            Core::FVector4 mScreenAndOpacity{};
            std::array<Core::Uint32, 4> mTextureFlags{};
        };

        static_assert(sizeof(FConstants) == 160);

    public:
        bool Initialize(ID3D11Device* Device, const D3D11_BLEND_DESC& ColorBlend);
        bool CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> VertexCode, std::span<const Core::Uint8> PixelCode);

        Core::Uint32 Render(ID3D11Device* Device, ID3D11DeviceContext* Context, Renderer::FDXRenderTarget& Target, const Engine::FSceneViewFamily& Family, std::span<const Engine::FDecalSceneProxy> Decals);

    private:
        static D3D11_RECT GetScissor(const Core::FMatrix& DecalToClip, Core::Uint32 Width, Core::Uint32 Height);

    private:
        RenderCore::TConstantBuffer<FConstants> mConstants{};
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mTestReceiver{};
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizer{};
        Microsoft::WRL::ComPtr<ID3D11SamplerState> mSampler{};
        Microsoft::WRL::ComPtr<ID3D11BlendState> mBlend{};
        Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};
        Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};
    };

}
