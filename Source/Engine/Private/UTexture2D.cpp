#include "Engine/UTexture2D.h"

#include <limits>
#include <utility>

using namespace Microsoft::WRL;

UTexture2D::UTexture2D() = default;

UTexture2D::~UTexture2D() = default;

bool UTexture2D::SetPixels(Uint32 SizeX, Uint32 SizeY, ETextureFormat Format, TArray<Uint8> Pixels) {
    if (SizeX == 0 || SizeY == 0 || SizeX > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || SizeY > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || (Format != ETextureFormat::RGBA8 && Format != ETextureFormat::BGRA8)) {
        return false;
    }

    constexpr std::size_t BytesPerPixel{4};
    const Uint64 PixelCount{static_cast<Uint64>(SizeX) * SizeY};

    if (PixelCount > std::numeric_limits<std::size_t>::max() / BytesPerPixel || Pixels.size() != static_cast<std::size_t>(PixelCount) * BytesPerPixel) {
        return false;
    }

    mSizeX = SizeX;
    mSizeY = SizeY;
    mFormat = Format;
    mPixels = std::move(Pixels);
    ++mRenderRevision;
    ReleaseResources();
    return true;
}

Uint32 UTexture2D::GetSizeX() const {
    return mSizeX;
}

Uint32 UTexture2D::GetSizeY() const {
    return mSizeY;
}

ETextureFormat UTexture2D::GetFormat() const {
    return mFormat;
}

const TArray<Uint8>& UTexture2D::GetPixels() const {
    return mPixels;
}

bool UTexture2D::IsSRGB() const {
    return mSRGB;
}

bool UTexture2D::SetSRGB(bool SRGB) {
    if (mSRGB != SRGB) {
        mSRGB = SRGB;
        ++mRenderRevision;
        ReleaseResources();
    }

    return true;
}

Uint64 UTexture2D::GetRenderRevision() const {
    return mRenderRevision;
}

bool UTexture2D::IsValid() const {
    return mSizeX != 0 && mSizeY != 0 && !mPixels.empty();
}

bool UTexture2D::CreateResources(ID3D11Device* Device) {
    if (Device == nullptr || !IsValid()) {
        return false;
    }

    if (HasResources()) {
        ComPtr<ID3D11Device> ResourceDevice{};
        mTexture->GetDevice(ResourceDevice.GetAddressOf());

        if (ResourceDevice.Get() == Device) {
            return true;
        }
    }

    DXGI_FORMAT ResourceFormat{DXGI_FORMAT_UNKNOWN};

    switch (mFormat) {
    case ETextureFormat::RGBA8:
        ResourceFormat = mSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        break;

    case ETextureFormat::BGRA8:
        ResourceFormat = mSRGB ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
        break;

    default:
        return false;
    }

    D3D11_TEXTURE2D_DESC Description{};
    Description.Width = mSizeX;
    Description.Height = mSizeY;
    Description.MipLevels = 1;
    Description.ArraySize = 1;
    Description.Format = ResourceFormat;
    Description.SampleDesc.Count = 1;
    Description.Usage = D3D11_USAGE_IMMUTABLE;
    Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA InitialData{};
    InitialData.pSysMem = mPixels.data();
    InitialData.SysMemPitch = mSizeX * 4;
    ComPtr<ID3D11Texture2D> Texture{};
    ComPtr<ID3D11ShaderResourceView> ShaderResourceView{};

    if (FAILED(Device->CreateTexture2D(&Description, &InitialData, Texture.GetAddressOf())) || FAILED(Device->CreateShaderResourceView(Texture.Get(), nullptr, ShaderResourceView.GetAddressOf()))) {
        return false;
    }

    mTexture = std::move(Texture);
    mShaderResourceView = std::move(ShaderResourceView);
    mResourceRevision = mRenderRevision;
    mResourceBytes = mPixels.size();
    return true;
}

void UTexture2D::ReleaseResources() {
    mShaderResourceView.Reset();
    mTexture.Reset();
    mResourceRevision = 0;
    mResourceBytes = 0;
}

bool UTexture2D::HasResources() const {
    return mResourceRevision == mRenderRevision && mTexture != nullptr && mShaderResourceView != nullptr;
}

ID3D11Texture2D* UTexture2D::GetTexture() const {
    return mTexture.Get();
}

ID3D11ShaderResourceView* UTexture2D::GetShaderResourceView() const {
    return mShaderResourceView.Get();
}

std::size_t UTexture2D::GetResourceBytes() const {
    return mResourceBytes;
}
