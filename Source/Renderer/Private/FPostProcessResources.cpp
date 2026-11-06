/**
 * @file FPostProcessResources.cpp
 * @brief 전체 화면 삼각형으로 안개를 합성하고 깊이 시각화 또는 FXAA를 최종 컬러에 출력합니다.
 * @ingroup Renderer
 */

#include "FPostProcessResources.h"
#include "FPostProcessConstants.h"
#include "RenderTarget.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/FHeightFogSceneProxy.h"

namespace Renderer {

    /**
     * @brief 기본 설정으로 FPostProcessResources 객체를 초기화합니다.
     */
    Renderer::FPostProcessResources::FPostProcessResources() = default;

    /**
     * @brief 객체 수명을 끝내고 소유한 자원을 정리합니다.
     */
    Renderer::FPostProcessResources::~FPostProcessResources() = default;

    /**
     * @brief 상수 버퍼·선형 Clamp 샘플러·깊이 비활성 상태와 면 제거 없는 래스터라이저를 생성합니다.
     */
    bool Renderer::FPostProcessResources::Initialize(ID3D11Device* Device) {
        D3D11_SAMPLER_DESC Sampler{};
        Sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        Sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        Sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        Sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        Sampler.MaxLOD = D3D11_FLOAT32_MAX;

        D3D11_DEPTH_STENCIL_DESC Depth{};
        Depth.DepthEnable = false;
        Depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        Depth.DepthFunc = D3D11_COMPARISON_ALWAYS;

        D3D11_RASTERIZER_DESC Rasterizer{};
        Rasterizer.FillMode = D3D11_FILL_SOLID;
        Rasterizer.CullMode = D3D11_CULL_NONE;
        Rasterizer.DepthClipEnable = true;

        Core::FString Error{};

        return mConstants.Create(Device, Error) && SUCCEEDED(Device->CreateSamplerState(&Sampler, mSampler.GetAddressOf())) && SUCCEEDED(Device->CreateDepthStencilState(&Depth, mDepthDisabled.GetAddressOf())) && SUCCEEDED(Device->CreateRasterizerState(&Rasterizer, mRasterizer.GetAddressOf()));
    }

    /**
     * @brief 정점·픽셀 바이트 코드로 후처리 셰이더를 생성하고 둘 다 성공한 경우에만 교체합니다.
     */
    bool Renderer::FPostProcessResources::CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> VertexCode, std::span<const Core::Uint8> PixelCode) {
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
     * @brief 가시 안개를 교대 타깃에 합성한 뒤 복사·깊이 시각화·FXAA 중 최종 패스를 실행합니다.
     */
    Core::Uint32 Renderer::FPostProcessResources::Render(ID3D11DeviceContext* Context, Renderer::FDXRenderTarget& Target, const Engine::FSceneViewFamily& Family, std::span<const Engine::FHeightFogSceneProxy> Fogs) {
        Renderer::FPostProcessConstants Constants{};

        if (!Family.mView.mViewProjection.TryInverse(Constants.mInverseViewProjection)) {
            Context->OMSetRenderTargets(0, nullptr, nullptr);
            Context->CopyResource(Target.GetOutputColor()->GetTexture(), Target.GetSceneColor()->GetTexture());

            return 0;
        }

        Constants.mView = Family.mView.mView;
        Constants.mCameraPosition = Core::FVector4{Family.mView.mLocation, Family.mView.mOrthographic ? 1.0f : 0.0f};
        Constants.mScreenSize = {static_cast<float>(Target.GetWidth()), static_cast<float>(Target.GetHeight()), 1.0f / static_cast<float>(Target.GetWidth()), 1.0f / static_cast<float>(Target.GetHeight())};

        Asset::UTexture2D* Source{Target.GetSceneColor()};
        Core::Uint32 DrawCalls{};
        const bool Visualization{Family.mViewMode == Engine::EViewMode::WorldNormal || Family.mViewMode == Engine::EViewMode::SceneDepth};

        if (!Visualization && Family.mShowFlags.mFog) {
            for (const Engine::FHeightFogSceneProxy& Fog : Fogs) {
                if (!Fog.mVisible || Fog.mDensity <= 0.0f || Fog.mMaxOpacity <= 0.0f) {
                    continue;
                }

                Constants.mFogColor = Fog.mInscatteringColor;
                Constants.mFogParameters = {Fog.mDensity, Fog.mHeightFalloff, Fog.mPosition.z, Fog.mMaxOpacity};
                Constants.mFogDistances = {Fog.mStartDistance, Fog.mEndDistance, Fog.mCutoffDistance, 0.0f};
                Constants.mSettings = {1.0f, 0.0f, 0.0f, 0.0f};

                mConstants.Update(Context, Constants);

                Asset::UTexture2D* Destination{Source == Target.GetSceneColor() ? Target.GetPostProcessColor() : Target.GetSceneColor()};

                Draw(Context, Source, Destination, Target.GetSceneDepth());

                Source = Destination;
                ++DrawCalls;
            }
        }

        Constants.mSettings = {Family.mViewMode == Engine::EViewMode::SceneDepth ? 2.0f : !Visualization && Family.mShowFlags.mFXAA ? 3.0f : 0.0f, 0.0f, 0.0f, 0.0f};

        mConstants.Update(Context, Constants);
        Draw(Context, Source, Target.GetOutputColor(), Target.GetSceneDepth());

        return DrawCalls + 1;
    }

    /**
     * @brief 입력 컬러·깊이를 읽어 다른 컬러 타깃에 전체 화면 삼각형을 그리고 SRV·RTV 바인딩을 해제합니다.
     */
    void Renderer::FPostProcessResources::Draw(ID3D11DeviceContext* Context, Asset::UTexture2D* Source, Asset::UTexture2D* Destination, Asset::UTexture2D* Depth) {
        const std::array<ID3D11ShaderResourceView*, 2> Empty{};

        Context->PSSetShaderResources(0, static_cast<UINT>(Empty.size()), Empty.data());

        ID3D11RenderTargetView* Output{Destination->GetRenderTargetView()};

        Context->OMSetRenderTargets(1, &Output, nullptr);
        Context->OMSetDepthStencilState(mDepthDisabled.Get(), 0);
        Context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
        Context->RSSetState(mRasterizer.Get());
        Context->IASetInputLayout(nullptr);
        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Context->VSSetShader(mVertexShader.Get(), nullptr, 0);
        Context->PSSetShader(mPixelShader.Get(), nullptr, 0);

        ID3D11SamplerState* Sampler{mSampler.Get()};
        const std::array<ID3D11ShaderResourceView*, 2> Resources{Source->GetShaderResourceView(), Depth->GetShaderResourceView()};

        mConstants.Bind(Context, RenderCore::EShaderStage::Pixel, 0);
        Context->PSSetSamplers(0, 1, &Sampler);
        Context->PSSetShaderResources(0, static_cast<UINT>(Resources.size()), Resources.data());
        Context->Draw(3, 0);
        Context->PSSetShaderResources(0, static_cast<UINT>(Empty.size()), Empty.data());
        Context->OMSetRenderTargets(0, nullptr, nullptr);
    }

}
