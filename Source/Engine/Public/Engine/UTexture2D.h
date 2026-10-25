#pragma once

#include "Engine/UTexture.h"
#include "Engine/ETextureUsage.h"

#include <d3d11.h>
#include <wrl/client.h>

class UTexture2D final : public UTexture {
public:
    ReflectTypes(UTexture2D, UTexture);

public:
    UTexture2D();
    ~UTexture2D() override;

public:
    ReflectClass(UTexture2D);

    bool Configure(Uint32 SizeX, Uint32 SizeY, ETextureFormat Format, ETextureUsage Usage, bool SRGB = false);
    bool SetPixels(Uint32 SizeX, Uint32 SizeY, ETextureFormat Format, TArray<Uint8> Pixels);
    Uint32 GetSizeX() const override;
    Uint32 GetSizeY() const override;
    ETextureFormat GetFormat() const override;
    const TArray<Uint8>& GetPixels() const;
    bool IsSRGB() const override;
    bool SetSRGB(bool SRGB);
    ETextureUsage GetUsage() const;
    bool SetUsage(ETextureUsage Usage);
    Uint64 GetRenderRevision() const override;
    bool IsValid() const override;
    bool CreateResources(ID3D11Device* Device);
    bool AttachResource(ID3D11Texture2D* Texture, ETextureUsage Usage);
    void ReleaseResources();
    bool HasResources() const;
    ID3D11Texture2D* GetTexture() const;
    ID3D11ShaderResourceView* GetShaderResourceView() const;
    ID3D11RenderTargetView* GetRenderTargetView() const;
    ID3D11DepthStencilView* GetDepthStencilView() const;
    std::size_t GetResourceBytes() const;

private:
    ReflectMembers();
    static bool IsValidSize(Uint32 SizeX, Uint32 SizeY);
    static bool IsDepthFormat(ETextureFormat Format);
    static bool ValidateUsage(ETextureFormat Format, ETextureUsage Usage, bool SRGB);
    static Uint32 GetBytesPerPixel(ETextureFormat Format);
    static Uint32 GetBindFlags(ETextureUsage Usage);
    static DXGI_FORMAT GetResourceFormat(ETextureFormat Format, bool SRGB);
    static DXGI_FORMAT GetShaderResourceFormat(ETextureFormat Format, bool SRGB);
    static DXGI_FORMAT GetDepthStencilFormat(ETextureFormat Format);
    static bool CreateViews(ID3D11Device* Device, ID3D11Texture2D* Texture, ETextureFormat Format, ETextureUsage Usage, bool SRGB, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& ShaderResourceView, Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& RenderTargetView, Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& DepthStencilView);

private:
    Uint32 mSizeX{};
    ReflectProperty(mSizeX, "SizeX", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mSizeY{};
    ReflectProperty(mSizeY, "SizeY", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    ETextureFormat mFormat{ETextureFormat::RGBA8};
    ReflectProperty(mFormat, "Format", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<Uint8> mPixels{};
    ReflectProperty(mPixels, "Pixels", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mSRGB{true};
    ReflectAccessor(mSRGB, IsSRGB, SetSRGB, "SRGB", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    ETextureUsage mUsage{ETextureUsage::ShaderResource};
    ReflectAccessor(mUsage, GetUsage, SetUsage, "Usage", EPropertyFlags::Edit | EPropertyFlags::Transient);
    Uint64 mRenderRevision{1};
    ReflectProperty(mRenderRevision, "RenderRevision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mTexture{};
    ReflectProperty(mTexture, "Texture", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mShaderResourceView{};
    ReflectProperty(mShaderResourceView, "ShaderResourceView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRenderTargetView{};
    ReflectProperty(mRenderTargetView, "RenderTargetView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDepthStencilView{};
    ReflectProperty(mDepthStencilView, "DepthStencilView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mExternalResource{};
    ReflectProperty(mExternalResource, "ExternalResource", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mResourceRevision{};
    ReflectProperty(mResourceRevision, "ResourceRevision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::size_t mResourceBytes{};
    ReflectProperty(mResourceBytes, "ResourceBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
