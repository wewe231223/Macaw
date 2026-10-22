#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FRenderTarget.h"

#include <d3d11.h>
#include <wrl/client.h>

class FDXRenderTarget final : public FRenderTarget {
public:
    ReflectTypes(FDXRenderTarget, FRenderTarget);

public:
    FDXRenderTarget();
    ~FDXRenderTarget() override;

public:
    ReflectStruct(FDXRenderTarget);

    bool Resize(ID3D11Device& Device, Uint32 Width, Uint32 Height);
    Uint32 GetWidth() const override;
    Uint32 GetHeight() const override;
    void* GetNativeTexture() const override;
    ID3D11RenderTargetView* GetRenderTargetView() const;
    ID3D11DepthStencilView* GetDepthView() const;

private:
    ReflectMembers();

private:
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mColor{};
    ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> mDepth{};
    ReflectProperty(mDepth, "Depth", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRenderTarget{};
    ReflectProperty(mRenderTarget, "RenderTarget", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mResourceView{};
    ReflectProperty(mResourceView, "ResourceView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDepthView{};
    ReflectProperty(mDepthView, "DepthView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    Uint32 mWidth{};
    ReflectProperty(mWidth, "Width", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Uint32 mHeight{};
    ReflectProperty(mHeight, "Height", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
