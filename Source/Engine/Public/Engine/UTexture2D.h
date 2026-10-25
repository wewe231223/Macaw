#pragma once

#include "Engine/UTexture.h"

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

    bool SetPixels(Uint32 SizeX, Uint32 SizeY, ETextureFormat Format, TArray<Uint8> Pixels);
    Uint32 GetSizeX() const override;
    Uint32 GetSizeY() const override;
    ETextureFormat GetFormat() const override;
    const TArray<Uint8>& GetPixels() const;
    bool IsSRGB() const override;
    bool SetSRGB(bool SRGB);
    Uint64 GetRenderRevision() const override;
    bool IsValid() const override;
    bool CreateResources(ID3D11Device* Device);
    void ReleaseResources();
    bool HasResources() const;
    ID3D11Texture2D* GetTexture() const;
    ID3D11ShaderResourceView* GetShaderResourceView() const;
    std::size_t GetResourceBytes() const;

private:
    ReflectMembers();

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
    Uint64 mRenderRevision{1};
    ReflectProperty(mRenderRevision, "RenderRevision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mTexture{};
    ReflectProperty(mTexture, "Texture", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mShaderResourceView{};
    ReflectProperty(mShaderResourceView, "ShaderResourceView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mResourceRevision{};
    ReflectProperty(mResourceRevision, "ResourceRevision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::size_t mResourceBytes{};
    ReflectProperty(mResourceBytes, "ResourceBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
