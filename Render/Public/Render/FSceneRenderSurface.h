#pragma once

#include <cstdint>
#include <d3d11.h>
#include <wrl/client.h>

class FSceneRenderSurface {
public:
    FSceneRenderSurface() = default;
    virtual ~FSceneRenderSurface() = default;

    FSceneRenderSurface(const FSceneRenderSurface&) = delete;
    FSceneRenderSurface& operator=(const FSceneRenderSurface&) = delete;
    FSceneRenderSurface(FSceneRenderSurface&&) = delete;
    FSceneRenderSurface& operator=(FSceneRenderSurface&&) = delete;

public:
    virtual bool Resize(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height) = 0;

    void Bind(ID3D11DeviceContext* Context) const;
    void Bind(ID3D11DeviceContext* Context, ID3D11DepthStencilView* DepthStencilView) const;
    void Clear(ID3D11DeviceContext* Context, const float ClearColor[4]) const;
    void ClearDepth(ID3D11DeviceContext* Context) const;

    virtual void Reset();
    bool IsValid() const;

    const D3D11_VIEWPORT& GetViewport() const;
    ID3D11ShaderResourceView* GetShaderResourceView() const;
    ID3D11ShaderResourceView* GetDepthShaderResourceView() const;
    ID3D11DepthStencilView* GetDepthStencilView() const;

private:
    friend class FOffScreenRenderSurface;
    friend class FSwapChainRenderSurface;

    void InitializeResources(ID3D11Device* Device, Microsoft::WRL::ComPtr<ID3D11Texture2D> ColorTexture, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ShaderResourceView);
    void CreateDepthStencilResources(ID3D11Device* Device, std::uint32_t Width, std::uint32_t Height);

private:
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mColorTexture{};
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRenderTargetView{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mShaderResourceView{};
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mDepthStencilTexture{};
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDepthStencilView{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mDepthShaderResourceView{};
    D3D11_VIEWPORT mViewport{};
};
