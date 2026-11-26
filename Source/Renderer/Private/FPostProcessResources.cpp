/**
 * @file FPostProcessResources.cpp
 * @brief 높이 안개, 피사계 심도, FXAA와 깊이 시각화를 순서대로 처리합니다.
 * @ingroup Renderer
 */

#include "FPostProcessResources.h"
#include "FPostProcessConstants.h"
#include "RenderTarget.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/FHeightFogSceneProxy.h"
#include "Core/Types.h"

#include <algorithm>

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
    bool Renderer::FPostProcessResources::CreateShaders(ID3D11Device* Device, std::span<const Uint8> VertexCode, std::span<const Uint8> PixelCode) {
        Microsoft::WRL::ComPtr<ID3D11VertexShader> Vertex{};
        Microsoft::WRL::ComPtr<ID3D11PixelShader> Pixel{};

        if (FAILED(Device->CreateVertexShader(VertexCode.data(), VertexCode.size(), nullptr, Vertex.GetAddressOf())) || FAILED(Device->CreatePixelShader(PixelCode.data(), PixelCode.size(), nullptr, Pixel.GetAddressOf()))) {
            return false;
        }

        mVertexShader = std::move(Vertex);
        mPixelShader = std::move(Pixel);

        return true;
    }

    bool Renderer::FPostProcessResources::CreateDepthOfFieldShaders(ID3D11Device* Device, std::span<const Uint8> SeparateCode, std::span<const Uint8> BlurCode, std::span<const Uint8> CompositeCode) {
        Microsoft::WRL::ComPtr<ID3D11PixelShader> Separate{};
        Microsoft::WRL::ComPtr<ID3D11PixelShader> Blur{};
        Microsoft::WRL::ComPtr<ID3D11PixelShader> Composite{};

        if (FAILED(Device->CreatePixelShader(SeparateCode.data(), SeparateCode.size(), nullptr, Separate.GetAddressOf())) ||
            FAILED(Device->CreatePixelShader(BlurCode.data(), BlurCode.size(), nullptr, Blur.GetAddressOf())) ||
            FAILED(Device->CreatePixelShader(CompositeCode.data(), CompositeCode.size(), nullptr, Composite.GetAddressOf()))) {
            return false;
        }

        mSeparateDepthOfField = std::move(Separate);
        mBlurDepthOfField = std::move(Blur);
        mCompositeDepthOfField = std::move(Composite);

        return true;
    }

    Uint32 Renderer::FPostProcessResources::Render(ID3D11DeviceContext* Context, Renderer::FDXRenderTarget& Target, const Engine::FSceneViewFamily& Family, std::span<const Engine::FHeightFogSceneProxy> Fogs) {
        Renderer::FPostProcessConstants Constants{};

        if (!Family.mView.mViewProjection.TryInverse(Constants.mInverseViewProjection)) {
            Context->OMSetRenderTargets(0, nullptr, nullptr);
            Context->CopyResource(Target.GetOutputColor()->GetTexture(), Target.GetSceneColor()->GetTexture());

            return 0;
        }

        Constants.mView = Family.mView.mView;
        Constants.mCameraPosition = FVector4{Family.mView.mLocation, Family.mView.mOrthographic ? 1.0f : 0.0f};
        Constants.mScreenSize = {static_cast<float>(Target.GetWidth()), static_cast<float>(Target.GetHeight()), 1.0f / static_cast<float>(Target.GetWidth()), 1.0f / static_cast<float>(Target.GetHeight())};

        const FMatrix& Projection{Family.mView.mProjection};
        Constants.mProjectionDepth = {Projection(2, 2), Projection(3, 2), Projection(2, 3), Projection(3, 3)};

        const Engine::FDepthOfFieldSettings& DepthOfField{Family.mView.mDepthOfFieldSettings};
        // 얇은 렌즈 모델: 초점거리 f, 조리개 지름 f/N, 초점 평면 S일 때 깊이 d의 착란원 지름은 센서에서
        // c(d) = f² / (N(S − f)) · |d − S| / d 입니다. 화면 높이가 센서 높이에 대응하므로 픽셀 반경은 c/2 · 화면 높이 / 센서 높이입니다.
        // 무한대에서의 반경 K만 CPU에서 구하고, 셰이더는 K · |d − S| / d로 픽셀마다 반경을 계산합니다.
        const float SensorHeight{Family.mView.mSensorHeight.GetBase()};
        const float FocalLength{Family.mView.mLensFocalLength > Core::FLength{} ? Family.mView.mLensFocalLength.GetBase() : Engine::Lens::FocalLengthFromFieldOfView(Family.mView.mFieldOfView, Family.mView.mSensorHeight).GetBase()};
        const float FocusDistance{std::max(DepthOfField.mFocalDistance.GetBase(), FocalLength * 1.001f)};
        const float MaxBlurRadius{std::max(DepthOfField.mMaxBlurRadius, 0.0f)};
        float InfinityBlurRadius{};

        if (FocalLength > 0.0f && SensorHeight > 0.0f && DepthOfField.mFStop > 0.0f && std::isfinite(FocalLength)) {
            const float InfinityCircleOfConfusion{FocalLength * FocalLength / (DepthOfField.mFStop * (FocusDistance - FocalLength))};
            InfinityBlurRadius = 0.5f * InfinityCircleOfConfusion / SensorHeight * static_cast<float>(Target.GetHeight());
        }

        // 블러는 픽셀마다 자신의 착란원 반경으로 퍼집니다. ZW는 레이어별 최대 탐색 반경입니다.
        // 배경 착란원은 무한대에서 K로 수렴하므로 K까지만 찾고, 전경은 카메라에 가까울수록 커지므로 상한까지 찾습니다.
        const float ForegroundBlurRadius{InfinityBlurRadius > 0.0f ? MaxBlurRadius : 0.0f};
        const float BackgroundBlurRadius{std::min(InfinityBlurRadius, MaxBlurRadius)};
        Constants.mDepthOfFieldParameters = {FocusDistance, InfinityBlurRadius, ForegroundBlurRadius, BackgroundBlurRadius};

        Asset::UTexture2D* Source{Target.GetSceneColor()};
        Uint32 DrawCalls{};
        const bool Visualization{Engine::IsBufferVisualization(Family.mViewMode)};

        if (!Visualization && Family.mShowFlags.mFog) {
            for (const Engine::FHeightFogSceneProxy& Fog : Fogs) {
                if (!Fog.mVisible || Fog.mDensity <= Core::FInverseLength{} || Fog.mMaxOpacity <= 0.0f) {
                    continue;
                }

                Constants.mFogColor = Fog.mInscatteringColor;
                Constants.mFogParameters = {Fog.mDensity.GetBase(), Fog.mHeightFalloff.GetBase(), Fog.mPosition.z, Fog.mMaxOpacity};
                Constants.mFogDistances = {Fog.mStartDistance.GetBase(), Fog.mEndDistance.GetBase(), Fog.mCutoffDistance.GetBase(), 0.0f};
                Constants.mSettings = {1.0f, 0.0f, 0.0f, 0.0f};

                mConstants.Update(Context, Constants);

                Asset::UTexture2D* Destination{Source == Target.GetSceneColor() ? Target.GetPostProcessColor() : Target.GetSceneColor()};

                Draw(Context, Source, Destination, Target.GetSceneDepth());

                Source = Destination;
                ++DrawCalls;
            }
        }

        Constants.mSettings.z = DepthOfField.mNearBlur ? 0.0f : 1.0f;
        Constants.mSettings.w = DepthOfField.mFarBlur ? 0.0f : 1.0f;

        if (mCompositeDepthOfField != nullptr && Family.mShowFlags.mDepthOfField && DepthOfField.mEnabled &&
            (DepthOfField.mNearBlur || DepthOfField.mFarBlur) && InfinityBlurRadius > 0.0f && MaxBlurRadius > 0.0f &&
            !Family.mView.mOrthographic && !Visualization && Family.mViewMode != Engine::EViewMode::Wireframe) {
            switch (DepthOfField.mApertureShape) {
            case Engine::EDepthOfFieldApertureShape::Pentagon:
                Constants.mSettings.y = 5.0f;
                break;
            case Engine::EDepthOfFieldApertureShape::Hexagon:
                Constants.mSettings.y = 6.0f;
                break;
            case Engine::EDepthOfFieldApertureShape::Circle:
                break;
            }

            Asset::UTexture2D* Destination{Source == Target.GetSceneColor() ? Target.GetPostProcessColor() : Target.GetSceneColor()};
            mConstants.Update(Context, Constants);

            if (RenderDepthOfField(Context, Target, Constants, *Source, *Destination)) {
                Source = Destination;
                DrawCalls += 4;
            }

            Constants.mSettings.x = 0.0f;
            Constants.mSettings.y = 0.0f;
        }

        if (Family.mViewMode == Engine::EViewMode::DepthOfFieldWeights) {
            Constants.mSettings.x = 4.0f;
        } else {
            Constants.mSettings.x = Family.mViewMode == Engine::EViewMode::SceneDepth ? 2.0f : !Visualization && Family.mShowFlags.mFXAA ? 3.0f : 0.0f;
        }

        // 조명은 선형 공간에서 계산하고 텍스처는 sRGB를 선형으로 디코딩해 읽으므로, 최종 출력에서 sRGB로 인코딩한다.
        // 버퍼 시각화(DOF 가중치 포함)와 깊이 시각화는 색이 아닌 데이터를 보여 주고, 와이어프레임은 색을 화면 값으로 직접 지정하므로 제외한다.
        // Settings.x, z, w는 위에서 정한 값(DOF 가중치 시각화, Near/Far 토글)을 유지하고 y만 최종 패스용 인코딩 플래그로 쓴다.
        // (DOF 패스에서는 y가 조리개 모양 값이었고 그 패스 뒤에 0으로 되돌린 상태다.)
        const bool EncodeSRGB{!Visualization && Family.mViewMode != Engine::EViewMode::SceneDepth && Family.mViewMode != Engine::EViewMode::Wireframe};

        Constants.mSettings.y = EncodeSRGB ? 1.0f : 0.0f;

        mConstants.Update(Context, Constants);
        Draw(Context, Source, Target.GetOutputColor(), Target.GetSceneDepth());

        return DrawCalls + 1;
    }

    bool Renderer::FPostProcessResources::RenderDepthOfField(ID3D11DeviceContext* Context, Renderer::FDXRenderTarget& Target, Renderer::FPostProcessConstants& Constants, const Asset::UTexture2D& Source, Asset::UTexture2D& Destination) {
        const FDXRenderTarget::FDepthOfFieldTargets* Layers{Target.PrepareDepthOfFieldTargets()};

        if (Layers == nullptr) {
            return false;
        }

        const auto& [Foreground, Background]{Layers->mSeparated};
        const auto& [BlurredForeground, BlurredBackground]{Layers->mBlurred};
        ID3D11ShaderResourceView* Color{Source.GetShaderResourceView()};
        ID3D11ShaderResourceView* Depth{Target.GetSceneDepth()->GetShaderResourceView()};

        const D3D11_VIEWPORT HalfViewport{0.0f, 0.0f, static_cast<float>(Foreground->GetSizeX()), static_cast<float>(Foreground->GetSizeY()), 0.0f, 1.0f};
        Context->RSSetViewports(1, &HalfViewport);

        // 분리 패스는 두 레이어와 함께 레이어별 착란원 반경을 기록합니다.
        const Core::TStaticArray<ID3D11RenderTargetView*, 3> Separated{Foreground->GetRenderTargetView(), Background->GetRenderTargetView(), Layers->mCircleOfConfusion->GetRenderTargetView()};
        const Core::TStaticArray<ID3D11ShaderResourceView*, 2> SceneInputs{Color, Depth};
        Draw(Context, mSeparateDepthOfField.Get(), Separated, SceneInputs);

        // 블러는 표본마다 자신의 착란원 반경으로 퍼지는 scatter-as-gather입니다. 레이어마다 탐색 반경이 달라 따로 실행합니다.
        // Settings.x가 블러할 레이어(0 전경, 1 배경)입니다. 셰이더는 그 레이어 하나만 출력하므로 해당 타깃만 0번 슬롯에 묶습니다.
        const Core::TStaticArray<ID3D11ShaderResourceView*, 5> BlurInputs{nullptr, nullptr, Foreground->GetShaderResourceView(), Background->GetShaderResourceView(), Layers->mCircleOfConfusion->GetShaderResourceView()};
        const Core::TStaticArray<ID3D11RenderTargetView*, 1> BlurredForegroundOnly{BlurredForeground->GetRenderTargetView()};
        const Core::TStaticArray<ID3D11RenderTargetView*, 1> BlurredBackgroundOnly{BlurredBackground->GetRenderTargetView()};

        Constants.mSettings.x = 0.0f;
        mConstants.Update(Context, Constants);
        Draw(Context, mBlurDepthOfField.Get(), BlurredForegroundOnly, BlurInputs);

        Constants.mSettings.x = 1.0f;
        mConstants.Update(Context, Constants);
        Draw(Context, mBlurDepthOfField.Get(), BlurredBackgroundOnly, BlurInputs);

        const D3D11_VIEWPORT FullViewport{0.0f, 0.0f, static_cast<float>(Target.GetWidth()), static_cast<float>(Target.GetHeight()), 0.0f, 1.0f};
        Context->RSSetViewports(1, &FullViewport);

        const Core::TStaticArray<ID3D11RenderTargetView*, 1> Composited{Destination.GetRenderTargetView()};
        const Core::TStaticArray<ID3D11ShaderResourceView*, 4> CompositeInputs{Color, Depth, BlurredForeground->GetShaderResourceView(), BlurredBackground->GetShaderResourceView()};
        Draw(Context, mCompositeDepthOfField.Get(), Composited, CompositeInputs);

        return true;
    }

    /**
     * @brief 입력 컬러·깊이를 읽어 다른 컬러 타깃에 전체 화면 삼각형을 그리고 SRV·RTV 바인딩을 해제합니다.
     */
    void Renderer::FPostProcessResources::Draw(ID3D11DeviceContext* Context, const Asset::UTexture2D* Source, Asset::UTexture2D* Destination, const Asset::UTexture2D* Depth) {
        const Core::TStaticArray<ID3D11RenderTargetView*, 1> Outputs{Destination->GetRenderTargetView()};
        const Core::TStaticArray<ID3D11ShaderResourceView*, 2> Inputs{Source->GetShaderResourceView(), Depth != nullptr ? Depth->GetShaderResourceView() : nullptr};
        Draw(Context, mPixelShader.Get(), Outputs, Inputs);
    }

    void Renderer::FPostProcessResources::Draw(ID3D11DeviceContext* Context, ID3D11PixelShader* PixelShader, std::span<ID3D11RenderTargetView* const> Outputs, std::span<ID3D11ShaderResourceView* const> Inputs, ID3D11BlendState* Blend) {
        // DOF 블러가 쓰는 t4(착란원 반경)까지 비웁니다.
        const Core::TStaticArray<ID3D11ShaderResourceView*, 5> Empty{};

        Context->PSSetShaderResources(0, static_cast<UINT>(Empty.size()), Empty.data());
        Context->OMSetRenderTargets(static_cast<UINT>(Outputs.size()), Outputs.data(), nullptr);
        Context->OMSetDepthStencilState(mDepthDisabled.Get(), 0);
        Context->OMSetBlendState(Blend, nullptr, 0xFFFFFFFFu);
        Context->RSSetState(mRasterizer.Get());
        Context->IASetInputLayout(nullptr);
        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Context->VSSetShader(mVertexShader.Get(), nullptr, 0);
        Context->PSSetShader(PixelShader, nullptr, 0);

        ID3D11SamplerState* Sampler{mSampler.Get()};
        mConstants.Bind(Context, RenderCore::EShaderStage::Pixel, 0);
        Context->PSSetSamplers(0, 1, &Sampler);
        Context->PSSetShaderResources(0, static_cast<UINT>(Inputs.size()), Inputs.data());
        Context->Draw(3, 0);
        Context->PSSetShaderResources(0, static_cast<UINT>(Empty.size()), Empty.data());
        Context->OMSetRenderTargets(0, nullptr, nullptr);
    }

    void FPostProcessResources::RenderTexture(ID3D11DeviceContext* Context, const Asset::UTexture2D& Source, Asset::UTexture2D& Destination, ID3D11BlendState* Blend) {
        Renderer::FPostProcessConstants Constants{};

        // sRGB 텍스처는 샘플링할 때 선형으로 디코딩되므로, 그대로 RGBA8 UNORM에 기록하면 원본보다 어둡게 보인다.
        // 텍스처 뷰어와 썸네일이 원본 색을 보이도록 sRGB 텍스처만 다시 인코딩한다. 선형 텍스처(노멀 맵 등)는 값을 그대로 보인다.
        Constants.mSettings = {0.0f, Source.IsSRGB() ? 1.0f : 0.0f, 0.0f, 0.0f};
        const Core::TStaticArray<ID3D11RenderTargetView*, 1> Outputs{Destination.GetRenderTargetView()};
        const Core::TStaticArray<ID3D11ShaderResourceView*, 1> Inputs{Source.GetShaderResourceView()};

        mConstants.Update(Context, Constants);
        Draw(Context, mPixelShader.Get(), Outputs, Inputs, Blend);
    }

}
