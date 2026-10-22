/**
 * @file RenderTarget.cpp
 * @brief Direct3D 11 오프스크린 렌더 타깃을 구현합니다.
 * @ingroup Renderer
 */

#include "RenderTarget.h"
#include "Core/Base/ErrorHandler.h"

/**
 * @brief FDXRenderTarget의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FDXRenderTarget::FDXRenderTarget() = default;

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FDXRenderTarget::~FDXRenderTarget() = default;

/**
 * @brief 새 크기에 맞추어 렌더 타깃 리소스를 생성하거나 교체합니다.
 */
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

/**
 * @brief 가로 픽셀 수를 반환합니다.
 */
Uint32 FDXRenderTarget::GetWidth() const {
    return mWidth;
}

/**
 * @brief 세로 픽셀 수를 반환합니다.
 */
Uint32 FDXRenderTarget::GetHeight() const {
    return mHeight;
}

/**
 * @brief UI에서 샘플링할 셰이더 리소스 뷰의 비소유 주소를 반환합니다.
 */
void* FDXRenderTarget::GetNativeTexture() const {
    return mResourceView.Get();
}

/**
 * @brief 컬러 출력용 Direct3D 11 렌더 타깃 뷰의 비소유 주소를 반환합니다.
 */
ID3D11RenderTargetView* FDXRenderTarget::GetRenderTargetView() const {
    return mRenderTarget.Get();
}

/**
 * @brief 깊이 스텐실 뷰를 반환합니다.
 */
ID3D11DepthStencilView* FDXRenderTarget::GetDepthView() const {
    return mDepthView.Get();
}
