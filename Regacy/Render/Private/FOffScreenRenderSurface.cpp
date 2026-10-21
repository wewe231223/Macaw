#include "pch.h"
#include "Render/FOffScreenRenderSurface.h"
#include "Core/Base/ErrorHandler.h"
#include <utility>

void FOffScreenRenderSurface::Initialize(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height, DXGI_FORMAT ColorFormat) {
    Reset();
    mColorFormat = ColorFormat;
    Resize(Device, Width, Height);
}

bool FOffScreenRenderSurface::Resize(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height) {
    if (Device == nullptr || Width == 0 || Height == 0) {
        return false;
    }

    const D3D11_VIEWPORT& Viewport{GetViewport()};

    if (IsValid() && Viewport.Width == static_cast<float>(Width) && Viewport.Height == static_cast<float>(Height)) {
        return true;
    }

    FSceneRenderSurface::Reset();
    CreateResources(Device, Width, Height);

    return IsValid();
}

void FOffScreenRenderSurface::Reset() {
    FSceneRenderSurface::Reset();
    mColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
}

void FOffScreenRenderSurface::CreateResources(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height) {
    D3D11_TEXTURE2D_DESC TextureDescription{};

    TextureDescription.Width = Width;
    TextureDescription.Height = Height;
    TextureDescription.MipLevels = 1;
    TextureDescription.ArraySize = 1;
    TextureDescription.Format = mColorFormat;
    TextureDescription.SampleDesc.Count = 1;
    TextureDescription.Usage = D3D11_USAGE_DEFAULT;
    TextureDescription.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> ColorTexture{};
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ShaderResourceView{};

    ErrorHandler::ReportHRESULT(Device->CreateTexture2D(&TextureDescription, nullptr, ColorTexture.GetAddressOf()), "[ FOffScreenRenderSurface ]", "Failed to create scene color texture.", ErrorHandler::EErrorLevel::Critical);
    ErrorHandler::ReportHRESULT(Device->CreateRenderTargetView(ColorTexture.Get(), nullptr, RenderTargetView.GetAddressOf()), "[ FOffScreenRenderSurface ]", "Failed to create scene render target view.", ErrorHandler::EErrorLevel::Critical);
    ErrorHandler::ReportHRESULT(Device->CreateShaderResourceView(ColorTexture.Get(), nullptr, ShaderResourceView.GetAddressOf()), "[ FOffScreenRenderSurface ]", "Failed to create scene shader resource view.", ErrorHandler::EErrorLevel::Critical);
    InitializeResources(Device, std::move(ColorTexture), std::move(RenderTargetView), std::move(ShaderResourceView));
}
