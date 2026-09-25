#pragma once

#include <cstdint>
#include <d3d11.h>
#include <wrl/client.h>

#include "IRenderSurface.h"

class FSceneRenderSurface final : public IRenderSurface {
public:
    FSceneRenderSurface() = default;
    ~FSceneRenderSurface() override = default;

    FSceneRenderSurface(const FSceneRenderSurface&) = delete;
    FSceneRenderSurface& operator=(const FSceneRenderSurface&) = delete;
    FSceneRenderSurface(FSceneRenderSurface&&) = delete;
    FSceneRenderSurface& operator=(FSceneRenderSurface&&) = delete;

    void InitializeSwapChain(ID3D11Device* Device, IDXGISwapChain* SwapChain);
    void InitializeOffscreen(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height);
    bool Resize(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height) override;
    void Bind(ID3D11DeviceContext* Context) const override;
    void Clear(ID3D11DeviceContext* Context, const float ClearColor[4]) const override;
    void ClearDepth(ID3D11DeviceContext* Context) const override;
    void Reset() override;
    bool IsValid() const override;
    const D3D11_VIEWPORT& GetViewport() const override;
    ID3D11ShaderResourceView* GetShaderResourceView() const override;

private:
    enum class EStorageMode : std::uint8_t { None, SwapChain, Offscreen };

    void CreateSwapChainResources(ID3D11Device* Device);
    void CreateOffscreenResources(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height);
    void CreateDepthStencilResources(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height);
    void ResetResources();

    EStorageMode mStorageMode{EStorageMode::None};
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain{};
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mColorTexture{};
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRenderTargetView{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mShaderResourceView{};
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mDepthStencilTexture{};
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDepthStencilView{};
    D3D11_VIEWPORT mViewport{};
};
