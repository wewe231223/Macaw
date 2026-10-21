#pragma once

#include "Engine/FRenderTarget.h"

#include <d3d11.h>
#include <wrl/client.h>

class FDXRenderTarget final : public FRenderTarget {
public:
    FDXRenderTarget();
    ~FDXRenderTarget() override;

public:
    bool Resize(ID3D11Device& Device, Uint32 Width, Uint32 Height);
    Uint32 GetWidth() const override;
    Uint32 GetHeight() const override;
    void* GetNativeTexture() const override;
    ID3D11RenderTargetView* GetRenderTargetView() const;
    ID3D11DepthStencilView* GetDepthView() const;

private:
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mColor{};
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mDepth{};

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRenderTarget{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mResourceView{};
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDepthView{};

    Uint32 mWidth{};
    Uint32 mHeight{};
};
