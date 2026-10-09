#include "pch.h"
#include "Render/FSceneRenderSurface.h"
#include "Core/Base/ErrorHandler.h"
#include <utility>

void FSceneRenderSurface::Bind(ID3D11DeviceContext* Context) const {
    Bind(Context, mDepthStencilView.Get());
}

void FSceneRenderSurface::Bind(ID3D11DeviceContext* Context, ID3D11DepthStencilView* DepthStencilView) const {
    if (Context == nullptr || !IsValid()) {
        return;
    }

    ID3D11RenderTargetView* TargetView{mRenderTargetView.Get()};

    Context->OMSetRenderTargets(1, &TargetView, DepthStencilView);
    Context->RSSetViewports(1, &mViewport);
}

void FSceneRenderSurface::Clear(ID3D11DeviceContext* Context, const float ClearColor[4]) const {
    if (Context == nullptr || !IsValid()) {
        return;
    }

    Context->ClearRenderTargetView(mRenderTargetView.Get(), ClearColor);
    Context->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void FSceneRenderSurface::ClearDepth(ID3D11DeviceContext* Context) const {
    if (Context == nullptr || !IsValid()) {
        return;
    }

    Context->ClearDepthStencilView(mDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void FSceneRenderSurface::Reset() {
    mDepthShaderResourceView.Reset();
    mDepthStencilView.Reset();
    mDepthStencilTexture.Reset();
    mShaderResourceView.Reset();
    mRenderTargetView.Reset();
    mColorTexture.Reset();
    mViewport = {};
}

bool FSceneRenderSurface::IsValid() const {
    return mRenderTargetView != nullptr && mDepthStencilView != nullptr && mDepthShaderResourceView != nullptr && mViewport.Width > 0.0f && mViewport.Height > 0.0f;
}

const D3D11_VIEWPORT& FSceneRenderSurface::GetViewport() const {
    return mViewport;
}

ID3D11ShaderResourceView* FSceneRenderSurface::GetShaderResourceView() const {
    return mShaderResourceView.Get();
}

ID3D11ShaderResourceView* FSceneRenderSurface::GetDepthShaderResourceView() const {
    return mDepthShaderResourceView.Get();
}

ID3D11DepthStencilView* FSceneRenderSurface::GetDepthStencilView() const {
    return mDepthStencilView.Get();
}

void FSceneRenderSurface::InitializeResources(ID3D11Device* Device, Microsoft::WRL::ComPtr<ID3D11Texture2D> ColorTexture, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ShaderResourceView) {
    D3D11_TEXTURE2D_DESC TextureDescription{};

    ColorTexture->GetDesc(&TextureDescription);
    mColorTexture = std::move(ColorTexture);
    mRenderTargetView = std::move(RenderTargetView);
    mShaderResourceView = std::move(ShaderResourceView);
    CreateDepthStencilResources(Device, TextureDescription.Width, TextureDescription.Height);
}

void FSceneRenderSurface::CreateDepthStencilResources(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height) {
    D3D11_TEXTURE2D_DESC TextureDescription{};

    TextureDescription.Width = Width;
    TextureDescription.Height = Height;
    TextureDescription.MipLevels = 1;
    TextureDescription.ArraySize = 1;
    TextureDescription.Format = DXGI_FORMAT_R24G8_TYPELESS;
    TextureDescription.SampleDesc.Count = 1;
    TextureDescription.Usage = D3D11_USAGE_DEFAULT;
    TextureDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    ErrorHandler::ReportHRESULT(Device->CreateTexture2D(&TextureDescription, nullptr, mDepthStencilTexture.GetAddressOf()), "[ FSceneRenderSurface ]", "Failed to create depth stencil texture.", ErrorHandler::EErrorLevel::Critical);

    D3D11_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDescription{};

    DepthStencilViewDescription.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    DepthStencilViewDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    ErrorHandler::ReportHRESULT(Device->CreateDepthStencilView(mDepthStencilTexture.Get(), &DepthStencilViewDescription, mDepthStencilView.GetAddressOf()), "[ FSceneRenderSurface ]", "Failed to create depth stencil view.", ErrorHandler::EErrorLevel::Critical);

    D3D11_SHADER_RESOURCE_VIEW_DESC DepthResourceDescription{};

    DepthResourceDescription.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    DepthResourceDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    DepthResourceDescription.Texture2D.MipLevels = 1;
    ErrorHandler::ReportHRESULT(Device->CreateShaderResourceView(mDepthStencilTexture.Get(), &DepthResourceDescription, mDepthShaderResourceView.GetAddressOf()), "[ FSceneRenderSurface ]", "Failed to create depth shader resource view.", ErrorHandler::EErrorLevel::Critical);
    mViewport = {0.0f, 0.0f, static_cast<float>(Width), static_cast<float>(Height), 0.0f, 1.0f};
}
