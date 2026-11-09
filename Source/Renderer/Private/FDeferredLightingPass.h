#pragma once

#include "Core/Math/FMatrix.h"
#include "RenderCore/TConstantBuffer.h"

#include <array>
#include <span>
#include <wrl/client.h>

namespace Engine {
    struct FSceneViewFamily;
}

namespace Renderer {

    class FDXRenderTarget;
    struct FPassTarget;

    class FDeferredLightingPass {
    private:
        struct FConstants {
            Core::FMatrix mInverseViewProjection{};
            std::array<float, 2> mInverseSize{};
            Core::Uint32 mViewMode{};
            Core::Uint32 mPadding{};
        };

        static_assert(sizeof(FConstants) == 80);

    public:
        bool Initialize(ID3D11Device* Device);
        bool CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> VertexCode, std::span<const Core::Uint8> PixelCode);

        Core::Uint32 Render(ID3D11DeviceContext* Context, const Renderer::FDXRenderTarget& Target, const Renderer::FPassTarget& Output, const Engine::FSceneViewFamily& Family, ID3D11RasterizerState* Rasterizer);

    private:
        RenderCore::TConstantBuffer<FConstants> mConstants{};
        Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};
        Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};
    };

}
