/**
 * @file UTexture2D.cpp
 * @brief 2차원 텍스처의 CPU 픽셀과 Direct3D 리소스 관리에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/UTexture2D.h"

#include <DirectXTex.h>
#include <cwctype>
#include <limits>
#include <utility>

namespace Engine {

    Engine::UTexture2D::UTexture2D() = default;

    Engine::UTexture2D::UTexture2D(const std::filesystem::path& ImagePath, bool SRGB) {
        LoadFromFile(ImagePath, SRGB);
    }

    Engine::UTexture2D::~UTexture2D() = default;

    bool Engine::UTexture2D::SetDDSData(std::span<const Core::Uint8> Data, bool SRGB) {
        DirectX::ScratchImage Image{};
        return !Data.empty() && SUCCEEDED(DirectX::LoadFromDDSMemory(Data.data(), Data.size(), DirectX::DDS_FLAGS_NONE, nullptr, Image)) && SetImageData(Image, SRGB);
    }

    const Core::TArray<Core::Uint8>& Engine::UTexture2D::GetDDSData() const {
        return mDDSData;
    }

    bool Engine::UTexture2D::LoadFromFile(const std::filesystem::path& ImagePath, bool SRGB) {
        if (ImagePath.empty()) {
            return false;
        }

        std::wstring Extension{ImagePath.extension().wstring()};

        for (wchar_t& Character : Extension) {
            Character = static_cast<wchar_t>(std::towlower(Character));
        }

        DirectX::ScratchImage SourceImage{};
        HRESULT Result{E_FAIL};

        if (Extension == L".dds") {
            Result = DirectX::LoadFromDDSFile(ImagePath.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, SourceImage);
        } else if (Extension == L".tga") {
            Result = DirectX::LoadFromTGAFile(ImagePath.c_str(), DirectX::TGA_FLAGS_NONE, nullptr, SourceImage);
        } else if (Extension == L".hdr") {
            Result = DirectX::LoadFromHDRFile(ImagePath.c_str(), nullptr, SourceImage);
        } else {
            const HRESULT InitializeResult{CoInitializeEx(nullptr, COINIT_MULTITHREADED)};

            if (FAILED(InitializeResult) && InitializeResult != RPC_E_CHANGED_MODE) {
                return false;
            }

            Result = DirectX::LoadFromWICFile(ImagePath.c_str(), DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, SourceImage);

            if (SUCCEEDED(InitializeResult)) {
                CoUninitialize();
            }
        }

        return SUCCEEDED(Result) && SetImageData(SourceImage, SRGB);
    }

    bool Engine::UTexture2D::Configure(Core::Uint32 SizeX, Core::Uint32 SizeY, Engine::ETextureFormat Format, Engine::ETextureUsage Usage, bool SRGB) {
        if (!IsValidSize(SizeX, SizeY) || !ValidateUsage(Format, Usage, SRGB)) {
            return false;
        }

        if (mSizeX == SizeX && mSizeY == SizeY && mFormat == Format && mUsage == Usage && mSRGB == SRGB && mPixels.empty() && !mExternalResource) {
            return true;
        }

        ReleaseResources();
        mSizeX = SizeX;
        mSizeY = SizeY;
        mMipLevels = 1;
        mFormat = Format;
        mUsage = Usage;
        mSRGB = SRGB;
        mPixels = Core::TArray<Core::Uint8>{};
        mDDSData = Core::TArray<Core::Uint8>{};
        mExternalResource = false;

        return true;
    }

    bool Engine::UTexture2D::SetPixels(Core::Uint32 SizeX, Core::Uint32 SizeY, Engine::ETextureFormat Format, Core::TArray<Core::Uint8> Pixels) {
        const bool SRGB{mSRGB && (Format == Engine::ETextureFormat::RGBA8 || Format == Engine::ETextureFormat::BGRA8)};

        if (!IsValidSize(SizeX, SizeY) || IsDepthFormat(Format) || !ValidateUsage(Format, mUsage, SRGB)) {
            return false;
        }

        const std::size_t BytesPerPixel{GetBytesPerPixel(Format)};
        const Core::Uint64 PixelCount{static_cast<Core::Uint64>(SizeX) * SizeY};

        if (PixelCount > std::numeric_limits<std::size_t>::max() / BytesPerPixel || Pixels.size() != static_cast<std::size_t>(PixelCount) * BytesPerPixel) {
            return false;
        }

        mSizeX = SizeX;
        mSizeY = SizeY;
        mMipLevels = 1;
        mFormat = Format;
        mPixels = std::move(Pixels);
        mDDSData = Core::TArray<Core::Uint8>{};
        mSRGB = SRGB;
        mExternalResource = false;
        ReleaseResources();

        return true;
    }

    Core::Uint32 Engine::UTexture2D::GetSizeX() const {
        return mSizeX;
    }

    Core::Uint32 Engine::UTexture2D::GetSizeY() const {
        return mSizeY;
    }

    Core::Uint32 Engine::UTexture2D::GetMipLevels() const {
        return mMipLevels;
    }

    Engine::ETextureFormat Engine::UTexture2D::GetFormat() const {
        return mFormat;
    }

    const Core::TArray<Core::Uint8>& Engine::UTexture2D::GetPixels() const {
        return mPixels;
    }

    bool Engine::UTexture2D::IsSRGB() const {
        return mSRGB;
    }

    bool Engine::UTexture2D::SetSRGB(bool SRGB) {
        if (!ValidateUsage(mFormat, mUsage, SRGB) || (mExternalResource && mSRGB != SRGB)) {
            return false;
        }

        if (mSRGB != SRGB) {
            if (!mDDSData.empty()) {
                DirectX::ScratchImage SourceImage{};
                return SUCCEEDED(DirectX::LoadFromDDSMemory(mDDSData.data(), mDDSData.size(), DirectX::DDS_FLAGS_NONE, nullptr, SourceImage)) && SetImageData(SourceImage, SRGB);
            }

            mSRGB = SRGB;
            ReleaseResources();
        }

        return true;
    }

    Engine::ETextureUsage Engine::UTexture2D::GetUsage() const {
        return mUsage;
    }

    bool Engine::UTexture2D::SetUsage(Engine::ETextureUsage Usage) {
        if (!ValidateUsage(mFormat, Usage, mSRGB) || (mExternalResource && mUsage != Usage)) {
            return false;
        }

        if (mUsage != Usage) {
            mUsage = Usage;
            ReleaseResources();
        }

        return true;
    }

    bool Engine::UTexture2D::IsValid() const {
        return IsValidSize(mSizeX, mSizeY) && ValidateUsage(mFormat, mUsage, mSRGB);
    }

    bool Engine::UTexture2D::CreateResources(ID3D11Device* Device) {
        if (Device == nullptr || !IsValid()) {
            return false;
        }

        if (HasResources()) {
            Microsoft::WRL::ComPtr<ID3D11Device> ResourceDevice{};
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
        Description.MipLevels = mMipLevels;
        Description.ArraySize = 1;
        Description.Format = GetResourceFormat(mFormat, mSRGB);
        Description.SampleDesc.Count = 1;
        Description.Usage = !mPixels.empty() && (mUsage & (Engine::ETextureUsage::RenderTarget | Engine::ETextureUsage::DepthStencil)) == Engine::ETextureUsage::None ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
        Description.BindFlags = GetBindFlags(mUsage);

        DirectX::ScratchImage SourceImage{};
        Core::TArray<D3D11_SUBRESOURCE_DATA> InitialData{};
        std::size_t ResourceBytes{static_cast<std::size_t>(mSizeX) * mSizeY * GetBytesPerPixel(mFormat)};

        if (!mDDSData.empty()) {
            if (FAILED(DirectX::LoadFromDDSMemory(mDDSData.data(), mDDSData.size(), DirectX::DDS_FLAGS_NONE, nullptr, SourceImage))) {
                return false;
            }

            InitialData.reserve(mMipLevels);

            for (Core::Uint32 MipLevel{}; MipLevel < mMipLevels; ++MipLevel) {
                const DirectX::Image* MipImage{SourceImage.GetImage(MipLevel, 0, 0)};

                if (MipImage == nullptr || MipImage->rowPitch > std::numeric_limits<Core::Uint32>::max() || MipImage->slicePitch > std::numeric_limits<Core::Uint32>::max()) {
                    return false;
                }

                InitialData.push_back({MipImage->pixels, static_cast<Core::Uint32>(MipImage->rowPitch), static_cast<Core::Uint32>(MipImage->slicePitch)});
            }

            ResourceBytes = SourceImage.GetPixelsSize();
        } else if (!mPixels.empty()) {
            InitialData.push_back({mPixels.data(), mSizeX * GetBytesPerPixel(mFormat), 0});
        }

        Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture{};
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ShaderResourceView{};
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView{};
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DepthStencilView{};

        if (FAILED(Device->CreateTexture2D(&Description, InitialData.empty() ? nullptr : InitialData.data(), Texture.GetAddressOf())) || !CreateViews(Device, Texture.Get(), mFormat, mUsage, mSRGB, ShaderResourceView, RenderTargetView, DepthStencilView)) {
            return false;
        }

        mTexture = std::move(Texture);
        mShaderResourceView = std::move(ShaderResourceView);
        mRenderTargetView = std::move(RenderTargetView);
        mDepthStencilView = std::move(DepthStencilView);
        mResourceBytes = ResourceBytes;

        return true;
    }

    bool Engine::UTexture2D::AttachResource(ID3D11Texture2D* Texture, Engine::ETextureUsage Usage) {
        if (Texture == nullptr) {
            return false;
        }

        if (Texture == mTexture.Get() && Usage == mUsage && HasResources()) {
            return true;
        }

        D3D11_TEXTURE2D_DESC Description{};
        Texture->GetDesc(&Description);

        Engine::ETextureFormat Format{};
        bool SRGB{};

        switch (Description.Format) {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            Format = Engine::ETextureFormat::RGBA8;
            SRGB = Description.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            break;

        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            Format = Engine::ETextureFormat::BGRA8;
            SRGB = Description.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            break;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            Format = Engine::ETextureFormat::RGBA16Float;
            break;

        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            Format = Engine::ETextureFormat::RGBA32Float;
            break;

        case DXGI_FORMAT_R32_FLOAT:
            Format = Engine::ETextureFormat::R32Float;
            break;

        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            Format = Engine::ETextureFormat::D24S8;
            break;

        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
            Format = Engine::ETextureFormat::D32Float;
            break;

        default:
            return false;
        }

        const Core::Uint32 RequiredFlags{GetBindFlags(Usage)};

        if (!IsValidSize(Description.Width, Description.Height) || !ValidateUsage(Format, Usage, SRGB) || Description.MipLevels != 1 || Description.ArraySize != 1 || Description.SampleDesc.Count != 1 || (Description.BindFlags & RequiredFlags) != RequiredFlags) {
            return false;
        }

        Microsoft::WRL::ComPtr<ID3D11Device> Device{};
        Texture->GetDevice(Device.GetAddressOf());

        Microsoft::WRL::ComPtr<ID3D11Texture2D> Resource{Texture};
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ShaderResourceView{};
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView{};
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DepthStencilView{};

        if (!CreateViews(Device.Get(), Resource.Get(), Format, Usage, SRGB, ShaderResourceView, RenderTargetView, DepthStencilView)) {
            return false;
        }

        mTexture = std::move(Resource);
        mShaderResourceView = std::move(ShaderResourceView);
        mRenderTargetView = std::move(RenderTargetView);
        mDepthStencilView = std::move(DepthStencilView);
        mSizeX = Description.Width;
        mSizeY = Description.Height;
        mMipLevels = Description.MipLevels;
        mFormat = Format;
        mUsage = Usage;
        mSRGB = SRGB;
        mPixels = Core::TArray<Core::Uint8>{};
        mDDSData = Core::TArray<Core::Uint8>{};
        mExternalResource = true;
        mResourceBytes = static_cast<std::size_t>(mSizeX) * mSizeY * GetBytesPerPixel(mFormat);

        return true;
    }

    void Engine::UTexture2D::ReleaseResources() {
        mDepthStencilView.Reset();
        mRenderTargetView.Reset();
        mShaderResourceView.Reset();
        mTexture.Reset();
        mResourceBytes = 0;
    }

    bool Engine::UTexture2D::HasResources() const {
        return mTexture != nullptr && ((mUsage & Engine::ETextureUsage::ShaderResource) == Engine::ETextureUsage::None || mShaderResourceView != nullptr) && ((mUsage & Engine::ETextureUsage::RenderTarget) == Engine::ETextureUsage::None || mRenderTargetView != nullptr) && ((mUsage & Engine::ETextureUsage::DepthStencil) == Engine::ETextureUsage::None || mDepthStencilView != nullptr);
    }

    ID3D11Texture2D* Engine::UTexture2D::GetTexture() const {
        return mTexture.Get();
    }

    ID3D11ShaderResourceView* Engine::UTexture2D::GetShaderResourceView() const {
        return mShaderResourceView.Get();
    }

    ID3D11RenderTargetView* Engine::UTexture2D::GetRenderTargetView() const {
        return mRenderTargetView.Get();
    }

    ID3D11DepthStencilView* Engine::UTexture2D::GetDepthStencilView() const {
        return mDepthStencilView.Get();
    }

    std::size_t Engine::UTexture2D::GetResourceBytes() const {
        return mResourceBytes;
    }

    bool Engine::UTexture2D::SetImageData(DirectX::ScratchImage& SourceImage, bool SRGB) {
        const DirectX::TexMetadata& Metadata{SourceImage.GetMetadata()};

        if (Metadata.dimension != DirectX::TEX_DIMENSION_TEXTURE2D || Metadata.arraySize != 1 || Metadata.depth != 1 || Metadata.IsCubemap() || Metadata.width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || Metadata.height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || !IsValidSize(static_cast<Core::Uint32>(Metadata.width), static_cast<Core::Uint32>(Metadata.height))) {
            return false;
        }

        if (DirectX::IsTypeless(Metadata.format)) {
            const DXGI_FORMAT UNormFormat{DirectX::MakeTypelessUNORM(Metadata.format)};
            const DXGI_FORMAT TypedFormat{DirectX::IsTypeless(UNormFormat) ? DirectX::MakeTypelessFLOAT(Metadata.format) : UNormFormat};

            if (DirectX::IsTypeless(TypedFormat) || !SourceImage.OverrideFormat(TypedFormat)) {
                return false;
            }
        }

        if (DirectX::IsCompressed(SourceImage.GetMetadata().format)) {
            DirectX::ScratchImage DecompressedImage{};
            const DirectX::Image* BaseImage{SourceImage.GetImage(0, 0, 0)};

            if (BaseImage == nullptr || FAILED(DirectX::Decompress(*BaseImage, DXGI_FORMAT_UNKNOWN, DecompressedImage))) {
                return false;
            }

            SourceImage = std::move(DecompressedImage);
        }

        Engine::ETextureFormat Format{Engine::ETextureFormat::RGBA8};

        switch (SourceImage.GetMetadata().format) {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            Format = Engine::ETextureFormat::BGRA8;
            break;

        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            Format = Engine::ETextureFormat::RGBA16Float;
            break;

        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            Format = Engine::ETextureFormat::RGBA32Float;
            break;

        case DXGI_FORMAT_R32_FLOAT:
            Format = Engine::ETextureFormat::R32Float;
            break;

        default:
            break;
        }

        SRGB = SRGB && (Format == Engine::ETextureFormat::RGBA8 || Format == Engine::ETextureFormat::BGRA8);

        if (!ValidateUsage(Format, mUsage, SRGB) || !SourceImage.OverrideFormat(SRGB ? DirectX::MakeSRGB(SourceImage.GetMetadata().format) : DirectX::MakeLinear(SourceImage.GetMetadata().format))) {
            return false;
        }

        const DXGI_FORMAT TargetFormat{GetResourceFormat(Format, SRGB)};

        if (SourceImage.GetMetadata().format != TargetFormat) {
            DirectX::ScratchImage ConvertedImage{};
            const DirectX::Image* BaseImage{SourceImage.GetImage(0, 0, 0)};

            if (BaseImage == nullptr || FAILED(DirectX::Convert(*BaseImage, TargetFormat, DirectX::TEX_FILTER_FORCE_NON_WIC, DirectX::TEX_THRESHOLD_DEFAULT, ConvertedImage))) {
                return false;
            }

            SourceImage = std::move(ConvertedImage);
        }

        const DirectX::Image* BaseImage{SourceImage.GetImage(0, 0, 0)};
        DirectX::ScratchImage MipChain{};

        if (BaseImage == nullptr) {
            return false;
        }

        const HRESULT MipResult{BaseImage->width == 1 && BaseImage->height == 1 ? MipChain.InitializeFromImage(*BaseImage) : DirectX::GenerateMipMaps(*BaseImage, DirectX::TEX_FILTER_LINEAR | DirectX::TEX_FILTER_FORCE_NON_WIC, 0, MipChain)};

        if (FAILED(MipResult)) {
            return false;
        }

        DirectX::Blob DDSBlob{};

        if (FAILED(DirectX::SaveToDDSMemory(MipChain.GetImages(), MipChain.GetImageCount(), MipChain.GetMetadata(), DirectX::DDS_FLAGS_FORCE_DX10_EXT, DDSBlob))) {
            return false;
        }

        const DirectX::Image* TopMip{MipChain.GetImage(0, 0, 0)};
        const Core::Uint8* DDSBytes{static_cast<const Core::Uint8*>(DDSBlob.GetBufferPointer())};
        Core::TArray<Core::Uint8> Pixels{TopMip->pixels, TopMip->pixels + TopMip->slicePitch};
        Core::TArray<Core::Uint8> DDSData{DDSBytes, DDSBytes + DDSBlob.GetBufferSize()};

        ReleaseResources();
        mSizeX = static_cast<Core::Uint32>(MipChain.GetMetadata().width);
        mSizeY = static_cast<Core::Uint32>(MipChain.GetMetadata().height);
        mMipLevels = static_cast<Core::Uint32>(MipChain.GetMetadata().mipLevels);
        mFormat = Format;
        mPixels = std::move(Pixels);
        mDDSData = std::move(DDSData);
        mSRGB = SRGB;
        mExternalResource = false;

        return true;
    }

    bool Engine::UTexture2D::IsValidSize(Core::Uint32 SizeX, Core::Uint32 SizeY) {
        return SizeX > 0 && SizeY > 0 && SizeX <= D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION && SizeY <= D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    }

    bool Engine::UTexture2D::IsDepthFormat(Engine::ETextureFormat Format) {
        return Format == Engine::ETextureFormat::D24S8 || Format == Engine::ETextureFormat::D32Float;
    }

    bool Engine::UTexture2D::ValidateUsage(Engine::ETextureFormat Format, Engine::ETextureUsage Usage, bool SRGB) {
        const Core::Uint8 Flags{static_cast<Core::Uint8>(Usage)};

        if (GetBytesPerPixel(Format) == 0 || (Flags & ~static_cast<Core::Uint8>(Engine::ETextureUsage::ShaderResource | Engine::ETextureUsage::RenderTarget | Engine::ETextureUsage::DepthStencil)) != 0 || (SRGB && Format != Engine::ETextureFormat::RGBA8 && Format != Engine::ETextureFormat::BGRA8)) {
            return false;
        }

        return IsDepthFormat(Format) ? (Usage & Engine::ETextureUsage::RenderTarget) == Engine::ETextureUsage::None : (Usage & Engine::ETextureUsage::DepthStencil) == Engine::ETextureUsage::None;
    }

    Core::Uint32 Engine::UTexture2D::GetBytesPerPixel(Engine::ETextureFormat Format) {
        switch (Format) {
        case Engine::ETextureFormat::RGBA8:
        case Engine::ETextureFormat::BGRA8:
        case Engine::ETextureFormat::R32Float:
        case Engine::ETextureFormat::D24S8:
        case Engine::ETextureFormat::D32Float:
            return 4;

        case Engine::ETextureFormat::RGBA16Float:
            return 8;

        case Engine::ETextureFormat::RGBA32Float:
            return 16;

        default:
            return 0;
        }
    }

    Core::Uint32 Engine::UTexture2D::GetBindFlags(Engine::ETextureUsage Usage) {
        Core::Uint32 Flags{};
        Flags |= (Usage & Engine::ETextureUsage::ShaderResource) != Engine::ETextureUsage::None ? D3D11_BIND_SHADER_RESOURCE : 0;
        Flags |= (Usage & Engine::ETextureUsage::RenderTarget) != Engine::ETextureUsage::None ? D3D11_BIND_RENDER_TARGET : 0;
        Flags |= (Usage & Engine::ETextureUsage::DepthStencil) != Engine::ETextureUsage::None ? D3D11_BIND_DEPTH_STENCIL : 0;

        return Flags;
    }

    DXGI_FORMAT Engine::UTexture2D::GetResourceFormat(Engine::ETextureFormat Format, bool SRGB) {
        switch (Format) {
        case Engine::ETextureFormat::RGBA8:
            return SRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

        case Engine::ETextureFormat::BGRA8:
            return SRGB ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;

        case Engine::ETextureFormat::RGBA16Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case Engine::ETextureFormat::RGBA32Float:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;

        case Engine::ETextureFormat::R32Float:
            return DXGI_FORMAT_R32_FLOAT;

        case Engine::ETextureFormat::D24S8:
            return DXGI_FORMAT_R24G8_TYPELESS;

        case Engine::ETextureFormat::D32Float:
            return DXGI_FORMAT_R32_TYPELESS;

        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    DXGI_FORMAT Engine::UTexture2D::GetShaderResourceFormat(Engine::ETextureFormat Format, bool SRGB) {
        if (Format == Engine::ETextureFormat::D24S8) {
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        }

        if (Format == Engine::ETextureFormat::D32Float) {
            return DXGI_FORMAT_R32_FLOAT;
        }

        return GetResourceFormat(Format, SRGB);
    }

    DXGI_FORMAT Engine::UTexture2D::GetDepthStencilFormat(Engine::ETextureFormat Format) {
        if (Format == Engine::ETextureFormat::D24S8) {
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        }

        return Format == Engine::ETextureFormat::D32Float ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_UNKNOWN;
    }

    bool Engine::UTexture2D::CreateViews(ID3D11Device* Device, ID3D11Texture2D* Texture, Engine::ETextureFormat Format, Engine::ETextureUsage Usage, bool SRGB, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& ShaderResourceView, Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& RenderTargetView, Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& DepthStencilView) {
        if ((Usage & Engine::ETextureUsage::ShaderResource) != Engine::ETextureUsage::None) {
            D3D11_TEXTURE2D_DESC TextureDescription{};
            Texture->GetDesc(&TextureDescription);

            D3D11_SHADER_RESOURCE_VIEW_DESC Description{};
            Description.Format = GetShaderResourceFormat(Format, SRGB);
            Description.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            Description.Texture2D.MipLevels = TextureDescription.MipLevels;

            if (FAILED(Device->CreateShaderResourceView(Texture, &Description, ShaderResourceView.GetAddressOf()))) {
                return false;
            }
        }

        if ((Usage & Engine::ETextureUsage::RenderTarget) != Engine::ETextureUsage::None) {
            D3D11_RENDER_TARGET_VIEW_DESC Description{};
            Description.Format = GetResourceFormat(Format, SRGB);
            Description.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

            if (FAILED(Device->CreateRenderTargetView(Texture, &Description, RenderTargetView.GetAddressOf()))) {
                return false;
            }
        }

        if ((Usage & Engine::ETextureUsage::DepthStencil) != Engine::ETextureUsage::None) {
            D3D11_DEPTH_STENCIL_VIEW_DESC Description{};
            Description.Format = GetDepthStencilFormat(Format);
            Description.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

            if (FAILED(Device->CreateDepthStencilView(Texture, &Description, DepthStencilView.GetAddressOf()))) {
                return false;
            }
        }

        return true;
    }

}
