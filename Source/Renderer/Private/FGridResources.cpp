/**
 * @file FGridResources.cpp
 * @brief 에디터 그리드를 픽셀 셰이더로 그리는 패스의 자원입니다.
 * @ingroup Renderer
 */

#include "FGridResources.h"
#include "FPassTarget.h"
#include "Engine/FSceneView.h"

#include <algorithm>
#include <cmath>

namespace Renderer {

    Renderer::FGridResources::FGridResources() = default;
    Renderer::FGridResources::~FGridResources() = default;

    /**
     * @brief 상수 버퍼·블렌드·깊이·래스터라이저 상태를 생성합니다.
     */
    bool Renderer::FGridResources::Initialize(ID3D11Device* Device) {
        D3D11_BLEND_DESC Blend{};
        Blend.RenderTarget[0].BlendEnable = true;
        Blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        Blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        Blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        Blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        Blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        Blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        Blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        D3D11_DEPTH_STENCIL_DESC Depth{};
        Depth.DepthEnable = true;
        Depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        Depth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

        D3D11_RASTERIZER_DESC Rasterizer{};
        Rasterizer.FillMode = D3D11_FILL_SOLID;
        Rasterizer.CullMode = D3D11_CULL_NONE;
        Rasterizer.DepthClipEnable = true;

        Core::FString Error{};

        return mConstants.Create(Device, Error) && SUCCEEDED(Device->CreateBlendState(&Blend, mBlend.GetAddressOf())) && SUCCEEDED(Device->CreateDepthStencilState(&Depth, mDepthState.GetAddressOf())) && SUCCEEDED(Device->CreateRasterizerState(&Rasterizer, mRasterizer.GetAddressOf()));
    }

    /**
     * @brief 정점·픽셀 바이트 코드로 그리드 셰이더를 생성하고 둘 다 성공한 경우에만 교체합니다.
     */
    bool Renderer::FGridResources::CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> VertexCode, std::span<const Core::Uint8> PixelCode) {
        Microsoft::WRL::ComPtr<ID3D11VertexShader> Vertex{};
        Microsoft::WRL::ComPtr<ID3D11PixelShader> Pixel{};

        if (FAILED(Device->CreateVertexShader(VertexCode.data(), VertexCode.size(), nullptr, Vertex.GetAddressOf())) || FAILED(Device->CreatePixelShader(PixelCode.data(), PixelCode.size(), nullptr, Pixel.GetAddressOf()))) {
            return false;
        }

        mVertexShader = std::move(Vertex);
        mPixelShader = std::move(Pixel);

        return true;
    }

    /**
     * @brief 불투명 깊이로 가려지는 그리드를 깊이 기록 없이 색상 타깃에 혼합합니다.
     */
    Core::Uint32 Renderer::FGridResources::Render(ID3D11DeviceContext* Context, const Renderer::FPassTarget& Target, const Engine::FSceneView& View, float Spacing) {
        if (mVertexShader == nullptr || mPixelShader == nullptr) {
            return 0;
        }

        // 직교 뷰는 화면 전체를 덮고, 원근 뷰는 카메라가 높을수록 멀리까지 보이도록 반지름을 정합니다.
        const float Radius{View.mOrthographic ? View.mOrthographicWidth * 4.0f : std::max(Spacing * 100.0f, std::abs(View.mLocation.z) * 10.0f)};
        const FConstants Constants{View.mViewProjection, {View.mLocation.x, View.mLocation.y, Spacing, Radius}};

        mConstants.Update(Context, Constants);
        Context->OMSetRenderTargets(1, &Target.mColor, Target.mDepth);
        Context->OMSetDepthStencilState(mDepthState.Get(), 0);
        Context->OMSetBlendState(mBlend.Get(), nullptr, 0xFFFFFFFFu);
        Context->RSSetViewports(1, &Target.mViewport);
        Context->RSSetState(mRasterizer.Get());
        Context->IASetInputLayout(nullptr);
        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Context->VSSetShader(mVertexShader.Get(), nullptr, 0);
        Context->PSSetShader(mPixelShader.Get(), nullptr, 0);

        mConstants.Bind(Context, RenderCore::EShaderStage::Vertex | RenderCore::EShaderStage::Pixel, 0);
        Context->Draw(6, 0);
        Context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);

        return 1;
    }

}
