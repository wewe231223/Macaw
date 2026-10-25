#include "Engine/UTexture2D.h"

#include <limits>
#include <utility>

using namespace Microsoft::WRL;

UTexture2D::UTexture2D() = default;

UTexture2D::~UTexture2D() = default;

bool UTexture2D::Configure(Uint32 SizeX, Uint32 SizeY, ETextureFormat Format, ETextureUsage Usage, bool SRGB) {
    if (!IsValidSize(SizeX, SizeY) || !ValidateUsage(Format, Usage, SRGB)) {
        return false;
    }

    if (mSizeX == SizeX && mSizeY == SizeY && mFormat == Format && mUsage == Usage && mSRGB == SRGB && mPixels.empty() && !mExternalResource) {
        return true;
    }

    ReleaseResources();
    mSizeX = SizeX;
    mSizeY = SizeY;
    mFormat = Format;
    mUsage = Usage;
    mSRGB = SRGB;
    mPixels = TArray<Uint8>{};
    mExternalResource = false;
    ++mRenderRevision;
    return true;
}

bool UTexture2D::SetPixels(Uint32 SizeX, Uint32 SizeY, ETextureFormat Format, TArray<Uint8> Pixels) {
    const bool SRGB{mSRGB && (Format == ETextureFormat::RGBA8 || Format == ETextureFormat::BGRA8)};

    if (!IsValidSize(SizeX, SizeY) || IsDepthFormat(Format) || !ValidateUsage(Format, mUsage, SRGB)) {
        return false;
    }

    const std::size_t BytesPerPixel{GetBytesPerPixel(Format)};
    const Uint64 PixelCount{static_cast<Uint64>(SizeX) * SizeY};

    if (PixelCount > std::numeric_limits<std::size_t>::max() / BytesPerPixel || Pixels.size() != static_cast<std::size_t>(PixelCount) * BytesPerPixel) {
        return false;
    }

    mSizeX = SizeX;
    mSizeY = SizeY;
    mFormat = Format;
    mPixels = std::move(Pixels);
    mSRGB = SRGB;
    mExternalResource = false;
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
    if (!ValidateUsage(mFormat, mUsage, SRGB) || (mExternalResource && mSRGB != SRGB)) {
        return false;
    }

    if (mSRGB != SRGB) {
        mSRGB = SRGB;
        ++mRenderRevision;
        ReleaseResources();
    }

    return true;
}

ETextureUsage UTexture2D::GetUsage() const {
    return mUsage;
}

bool UTexture2D::SetUsage(ETextureUsage Usage) {
    if (!ValidateUsage(mFormat, Usage, mSRGB) || (mExternalResource && mUsage != Usage)) {
        return false;
    }

    if (mUsage != Usage) {
        mUsage = Usage;
        ++mRenderRevision;
        ReleaseResources();
    }

    return true;
}

Uint64 UTexture2D::GetRenderRevision() const {
    return mRenderRevision;
}

bool UTexture2D::IsValid() const {
    return IsValidSize(mSizeX, mSizeY) && ValidateUsage(mFormat, mUsage, mSRGB);
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

    if (mExternalResource) {
        return false;
    }

    D3D11_TEXTURE2D_DESC Description{};
    Description.Width = mSizeX;
    Description.Height = mSizeY;
    Description.MipLevels = 1;
    Description.ArraySize = 1;
    Description.Format = GetResourceFormat(mFormat, mSRGB);
    Description.SampleDesc.Count = 1;
    Description.Usage = !mPixels.empty() && (mUsage & (ETextureUsage::RenderTarget | ETextureUsage::DepthStencil)) == ETextureUsage::None ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
    Description.BindFlags = GetBindFlags(mUsage);

    D3D11_SUBRESOURCE_DATA InitialData{};
    InitialData.pSysMem = mPixels.data();
    InitialData.SysMemPitch = mSizeX * GetBytesPerPixel(mFormat);
    ComPtr<ID3D11Texture2D> Texture{};
    ComPtr<ID3D11ShaderResourceView> ShaderResourceView{};
    ComPtr<ID3D11RenderTargetView> RenderTargetView{};
    ComPtr<ID3D11DepthStencilView> DepthStencilView{};

    if (FAILED(Device->CreateTexture2D(&Description, mPixels.empty() ? nullptr : &InitialData, Texture.GetAddressOf())) || !CreateViews(Device, Texture.Get(), mFormat, mUsage, mSRGB, ShaderResourceView, RenderTargetView, DepthStencilView)) {
        return false;
    }

    mTexture = std::move(Texture);
    mShaderResourceView = std::move(ShaderResourceView);
    mRenderTargetView = std::move(RenderTargetView);
    mDepthStencilView = std::move(DepthStencilView);
    mResourceRevision = mRenderRevision;
    mResourceBytes = static_cast<std::size_t>(mSizeX) * mSizeY * GetBytesPerPixel(mFormat);
    return true;
}

bool UTexture2D::AttachResource(ID3D11Texture2D* Texture, ETextureUsage Usage) {
    if (Texture == nullptr) {
        return false;
    }

    if (Texture == mTexture.Get() && Usage == mUsage && HasResources()) {
        return true;
    }

    D3D11_TEXTURE2D_DESC Description{};
    Texture->GetDesc(&Description);
    ETextureFormat Format{};
    bool SRGB{};

    switch (Description.Format) {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        Format = ETextureFormat::RGBA8;
        SRGB = Description.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        break;

    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        Format = ETextureFormat::BGRA8;
        SRGB = Description.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        break;

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        Format = ETextureFormat::RGBA16Float;
        break;

    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        Format = ETextureFormat::RGBA32Float;
        break;

    case DXGI_FORMAT_R32_FLOAT:
        Format = ETextureFormat::R32Float;
        break;

    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        Format = ETextureFormat::D24S8;
        break;

    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
        Format = ETextureFormat::D32Float;
        break;

    default:
        return false;
    }

    const Uint32 RequiredFlags{GetBindFlags(Usage)};

    if (!IsValidSize(Description.Width, Description.Height) || !ValidateUsage(Format, Usage, SRGB) || Description.MipLevels != 1 || Description.ArraySize != 1 || Description.SampleDesc.Count != 1 || (Description.BindFlags & RequiredFlags) != RequiredFlags) {
        return false;
    }

    ComPtr<ID3D11Device> Device{};
    Texture->GetDevice(Device.GetAddressOf());
    ComPtr<ID3D11Texture2D> Resource{Texture};
    ComPtr<ID3D11ShaderResourceView> ShaderResourceView{};
    ComPtr<ID3D11RenderTargetView> RenderTargetView{};
    ComPtr<ID3D11DepthStencilView> DepthStencilView{};

    if (!CreateViews(Device.Get(), Resource.Get(), Format, Usage, SRGB, ShaderResourceView, RenderTargetView, DepthStencilView)) {
        return false;
    }

    mTexture = std::move(Resource);
    mShaderResourceView = std::move(ShaderResourceView);
    mRenderTargetView = std::move(RenderTargetView);
    mDepthStencilView = std::move(DepthStencilView);
    mSizeX = Description.Width;
    mSizeY = Description.Height;
    mFormat = Format;
    mUsage = Usage;
    mSRGB = SRGB;
    mPixels = TArray<Uint8>{};
    mExternalResource = true;
    ++mRenderRevision;
    mResourceRevision = mRenderRevision;
    mResourceBytes = static_cast<std::size_t>(mSizeX) * mSizeY * GetBytesPerPixel(mFormat);
    return true;
}

void UTexture2D::ReleaseResources() {
    mDepthStencilView.Reset();
    mRenderTargetView.Reset();
    mShaderResourceView.Reset();
    mTexture.Reset();
    mResourceRevision = 0;
    mResourceBytes = 0;
}

bool UTexture2D::HasResources() const {
    return mResourceRevision == mRenderRevision && mTexture != nullptr && ((mUsage & ETextureUsage::ShaderResource) == ETextureUsage::None || mShaderResourceView != nullptr) && ((mUsage & ETextureUsage::RenderTarget) == ETextureUsage::None || mRenderTargetView != nullptr) && ((mUsage & ETextureUsage::DepthStencil) == ETextureUsage::None || mDepthStencilView != nullptr);
}

ID3D11Texture2D* UTexture2D::GetTexture() const {
    return mTexture.Get();
}

ID3D11ShaderResourceView* UTexture2D::GetShaderResourceView() const {
    return mShaderResourceView.Get();
}

ID3D11RenderTargetView* UTexture2D::GetRenderTargetView() const {
    return mRenderTargetView.Get();
}

ID3D11DepthStencilView* UTexture2D::GetDepthStencilView() const {
    return mDepthStencilView.Get();
}

std::size_t UTexture2D::GetResourceBytes() const {
    return mResourceBytes;
}

bool UTexture2D::IsValidSize(Uint32 SizeX, Uint32 SizeY) {
    return SizeX > 0 && SizeY > 0 && SizeX <= D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION && SizeY <= D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
}

bool UTexture2D::IsDepthFormat(ETextureFormat Format) {
    return Format == ETextureFormat::D24S8 || Format == ETextureFormat::D32Float;
}

bool UTexture2D::ValidateUsage(ETextureFormat Format, ETextureUsage Usage, bool SRGB) {
    const Uint8 Flags{static_cast<Uint8>(Usage)};

    if (GetBytesPerPixel(Format) == 0 || (Flags & ~static_cast<Uint8>(ETextureUsage::ShaderResource | ETextureUsage::RenderTarget | ETextureUsage::DepthStencil)) != 0 || (SRGB && Format != ETextureFormat::RGBA8 && Format != ETextureFormat::BGRA8)) {
        return false;
    }

    return IsDepthFormat(Format) ? (Usage & ETextureUsage::RenderTarget) == ETextureUsage::None : (Usage & ETextureUsage::DepthStencil) == ETextureUsage::None;
}

Uint32 UTexture2D::GetBytesPerPixel(ETextureFormat Format) {
    switch (Format) {
    case ETextureFormat::RGBA8:
    case ETextureFormat::BGRA8:
    case ETextureFormat::R32Float:
    case ETextureFormat::D24S8:
    case ETextureFormat::D32Float:
        return 4;

    case ETextureFormat::RGBA16Float:
        return 8;

    case ETextureFormat::RGBA32Float:
        return 16;

    default:
        return 0;
    }
}

Uint32 UTexture2D::GetBindFlags(ETextureUsage Usage) {
    Uint32 Flags{};
    Flags |= (Usage & ETextureUsage::ShaderResource) != ETextureUsage::None ? D3D11_BIND_SHADER_RESOURCE : 0;
    Flags |= (Usage & ETextureUsage::RenderTarget) != ETextureUsage::None ? D3D11_BIND_RENDER_TARGET : 0;
    Flags |= (Usage & ETextureUsage::DepthStencil) != ETextureUsage::None ? D3D11_BIND_DEPTH_STENCIL : 0;
    return Flags;
}

DXGI_FORMAT UTexture2D::GetResourceFormat(ETextureFormat Format, bool SRGB) {
    switch (Format) {
    case ETextureFormat::RGBA8:
        return SRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

    case ETextureFormat::BGRA8:
        return SRGB ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;

    case ETextureFormat::RGBA16Float:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;

    case ETextureFormat::RGBA32Float:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;

    case ETextureFormat::R32Float:
        return DXGI_FORMAT_R32_FLOAT;

    case ETextureFormat::D24S8:
        return DXGI_FORMAT_R24G8_TYPELESS;

    case ETextureFormat::D32Float:
        return DXGI_FORMAT_R32_TYPELESS;

    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT UTexture2D::GetShaderResourceFormat(ETextureFormat Format, bool SRGB) {
    if (Format == ETextureFormat::D24S8) {
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    }

    if (Format == ETextureFormat::D32Float) {
        return DXGI_FORMAT_R32_FLOAT;
    }

    return GetResourceFormat(Format, SRGB);
}

DXGI_FORMAT UTexture2D::GetDepthStencilFormat(ETextureFormat Format) {
    if (Format == ETextureFormat::D24S8) {
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    }

    return Format == ETextureFormat::D32Float ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_UNKNOWN;
}

bool UTexture2D::CreateViews(ID3D11Device* Device, ID3D11Texture2D* Texture, ETextureFormat Format, ETextureUsage Usage, bool SRGB, ComPtr<ID3D11ShaderResourceView>& ShaderResourceView, ComPtr<ID3D11RenderTargetView>& RenderTargetView, ComPtr<ID3D11DepthStencilView>& DepthStencilView) {
    if ((Usage & ETextureUsage::ShaderResource) != ETextureUsage::None) {
        D3D11_SHADER_RESOURCE_VIEW_DESC Description{};
        Description.Format = GetShaderResourceFormat(Format, SRGB);
        Description.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        Description.Texture2D.MipLevels = 1;

        if (FAILED(Device->CreateShaderResourceView(Texture, &Description, ShaderResourceView.GetAddressOf()))) {
            return false;
        }
    }

    if ((Usage & ETextureUsage::RenderTarget) != ETextureUsage::None) {
        D3D11_RENDER_TARGET_VIEW_DESC Description{};
        Description.Format = GetResourceFormat(Format, SRGB);
        Description.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

        if (FAILED(Device->CreateRenderTargetView(Texture, &Description, RenderTargetView.GetAddressOf()))) {
            return false;
        }
    }

    if ((Usage & ETextureUsage::DepthStencil) != ETextureUsage::None) {
        D3D11_DEPTH_STENCIL_VIEW_DESC Description{};
        Description.Format = GetDepthStencilFormat(Format);
        Description.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

        if (FAILED(Device->CreateDepthStencilView(Texture, &Description, DepthStencilView.GetAddressOf()))) {
            return false;
        }
    }

    return true;
}
