#include "RenderTarget.h"
#include "Core/Base/ErrorHandler.h"

FDXRenderTarget::FDXRenderTarget() = default;

FDXRenderTarget::~FDXRenderTarget() = default;

bool FDXRenderTarget::Resize(ID3D11Device& Device, Uint32 Width, Uint32 Height) {
    if (Width == 0 || Height == 0 || Width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || Height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION) {
        return false;
    }

    if (Width == mWidth && Height == mHeight) {
        return true;
    }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> Color{};
    Microsoft::WRL::ComPtr<ID3D11Texture2D> Depth{};
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTarget{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ResourceView{};
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DepthView{};

    D3D11_TEXTURE2D_DESC Description{};
    Description.Width = Width;
    Description.Height = Height;
    Description.MipLevels = 1;
    Description.ArraySize = 1;
    Description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    Description.SampleDesc.Count = 1;
    Description.Usage = D3D11_USAGE_DEFAULT;
    Description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(Device.CreateTexture2D(&Description, nullptr, Color.GetAddressOf())) || FAILED(Device.CreateRenderTargetView(Color.Get(), nullptr, RenderTarget.GetAddressOf())) || FAILED(Device.CreateShaderResourceView(Color.Get(), nullptr, ResourceView.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Render target allocation failed");
    }

    Description.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    Description.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    if (FAILED(Device.CreateTexture2D(&Description, nullptr, Depth.GetAddressOf())) || FAILED(Device.CreateDepthStencilView(Depth.Get(), nullptr, DepthView.GetAddressOf()))) {
        ErrorHandler::Fatal("Renderer", "Depth target allocation failed");
    }

    mColor = std::move(Color);
    mDepth = std::move(Depth);
    mRenderTarget = std::move(RenderTarget);
    mResourceView = std::move(ResourceView);
    mDepthView = std::move(DepthView);

    mWidth = Width;
    mHeight = Height;

    return true;
}

Uint32 FDXRenderTarget::GetWidth() const {
    return mWidth;
}

Uint32 FDXRenderTarget::GetHeight() const {
    return mHeight;
}

void* FDXRenderTarget::GetNativeTexture() const {
    return mResourceView.Get();
}

ID3D11RenderTargetView* FDXRenderTarget::GetRenderTargetView() const {
    return mRenderTarget.Get();
}

ID3D11DepthStencilView* FDXRenderTarget::GetDepthView() const {
    return mDepthView.Get();
}
