/**
 * @file UTexture2D.cpp
 * @brief 2차원 텍스처의 CPU 픽셀과 Direct3D 리소스 관리에 대한 구현입니다.
 * @ingroup Asset
 */

#include "Asset/UTexture2D.h"
#include "Core/FilePath.h"
#include "Core/LoadingStatus.h"

#include <DirectXTex.h>
#include <fstream>
#include <limits>
#include <utility>

namespace Asset {

    Asset::UTexture2D::UTexture2D() = default;

    /**
     * @brief 전달받은 설정과 의존성으로 UTexture2D의 초기 상태를 구성합니다.
     */
    Asset::UTexture2D::UTexture2D(const std::filesystem::path& ImagePath, bool SRGB) {
        LoadFromFile(ImagePath, SRGB);
    }

    Asset::UTexture2D::~UTexture2D() = default;

    /**
     * @brief DDS 데이터를 읽어 이미지 형식과 크기 및 CPU 이미지 데이터를 설정합니다.
     */
    bool Asset::UTexture2D::SetDDSData(std::span<const Core::Uint8> Data, bool SRGB) {
        return LoadFromMemory(Data, L"Image.dds", SRGB);
    }

    /**
     * @brief 보존 중인 DDS 형식의 이미지 바이트 배열을 반환합니다.
     */
    const Core::TArray<Core::Uint8>& Asset::UTexture2D::GetDDSData() const {
        return mDDSData;
    }

    /**
     * @brief 이미지 파일을 디코딩하여 텍스처 크기·형식·픽셀 데이터를 설정합니다.
     */
    bool Asset::UTexture2D::LoadFromFile(const std::filesystem::path& ImagePath, bool SRGB) {
        Core::LoadingStatus::ReportFile("Importing", ImagePath);

        std::error_code Code{};
        const auto Size{ImagePath.empty() ? 0 : std::filesystem::file_size(ImagePath, Code)};
        std::ifstream Input{ImagePath, std::ios::binary};

        if (Code || Size == 0 || !Input) {
            return false;
        }

        Core::TArray<Core::Uint8> Data(static_cast<std::size_t>(Size));

        return Input.read(reinterpret_cast<char*>(Data.data()), static_cast<std::streamsize>(Data.size())) && LoadFromMemory(Data, ImagePath, SRGB);
    }

    /**
     * @brief 메모리의 이미지 바이트를 디코딩하여 텍스처 크기·형식·픽셀 데이터를 설정합니다.
     */
    bool Asset::UTexture2D::LoadFromMemory(std::span<const Core::Uint8> Data, const std::filesystem::path& ImagePath, bool SRGB) {
        if (Data.empty()) {
            return false;
        }

        const std::wstring Extension{Core::FilePath::ToLowerExtension(ImagePath)};
        DirectX::ScratchImage SourceImage{};
        HRESULT Result{E_FAIL};

        if (Extension == L".dds") {
            Result = DirectX::LoadFromDDSMemory(Data.data(), Data.size(), DirectX::DDS_FLAGS_NONE, nullptr, SourceImage);
        } else if (Extension == L".tga") {
            Result = DirectX::LoadFromTGAMemory(Data.data(), Data.size(), DirectX::TGA_FLAGS_NONE, nullptr, SourceImage);
        } else if (Extension == L".hdr") {
            Result = DirectX::LoadFromHDRMemory(Data.data(), Data.size(), nullptr, SourceImage);
        } else {
            const HRESULT InitializeResult{CoInitializeEx(nullptr, COINIT_MULTITHREADED)};

            if (FAILED(InitializeResult) && InitializeResult != RPC_E_CHANGED_MODE) {
                return false;
            }

            Result = DirectX::LoadFromWICMemory(Data.data(), Data.size(), DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, SourceImage);

            if (SUCCEEDED(InitializeResult)) {
                CoUninitialize();
            }
        }

        return SUCCEEDED(Result) && SetImageData(SourceImage, SRGB);
    }

    /**
     * @brief 크기·형식·용도·색 공간을 검증하여 텍스처 생성 구성을 설정합니다.
     */
    bool Asset::UTexture2D::Configure(Core::Uint32 SizeX, Core::Uint32 SizeY, Asset::ETextureFormat Format, Asset::ETextureUsage Usage, bool SRGB) {
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

    /**
     * @brief 크기와 형식에 맞는 CPU 픽셀 데이터를 검증하여 교체합니다.
     */
    bool Asset::UTexture2D::SetPixels(Core::Uint32 SizeX, Core::Uint32 SizeY, Asset::ETextureFormat Format, Core::TArray<Core::Uint8> Pixels) {
        const bool SRGB{mSRGB && (Format == Asset::ETextureFormat::RGBA8 || Format == Asset::ETextureFormat::BGRA8)};

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

    /**
     * @brief 텍스처의 가로 픽셀 수를 반환합니다.
     */
    Core::Uint32 Asset::UTexture2D::GetSizeX() const {
        return mSizeX;
    }

    /**
     * @brief 텍스처의 세로 픽셀 수를 반환합니다.
     */
    Core::Uint32 Asset::UTexture2D::GetSizeY() const {
        return mSizeY;
    }

    /**
     * @brief 텍스처의 밉 레벨 수를 반환합니다.
     */
    Core::Uint32 Asset::UTexture2D::GetMipLevels() const {
        return mMipLevels;
    }

    /**
     * @brief 텍스처의 픽셀 또는 깊이 형식을 반환합니다.
     */
    Asset::ETextureFormat Asset::UTexture2D::GetFormat() const {
        return mFormat;
    }

    /**
     * @brief CPU 메모리에 보관한 픽셀 바이트 배열을 반환합니다.
     */
    const Core::TArray<Core::Uint8>& Asset::UTexture2D::GetPixels() const {
        return mPixels;
    }

    /**
     * @brief 텍스처를 sRGB 색 공간으로 해석하는지 반환합니다.
     */
    bool Asset::UTexture2D::IsSRGB() const {
        return mSRGB;
    }

    /**
     * @brief 현재 텍스처 형식과 용도에 맞는지 검증하여 sRGB 설정을 변경합니다.
     */
    bool Asset::UTexture2D::SetSRGB(bool SRGB) {
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

    /**
     * @brief 텍스처에 설정된 GPU 사용 용도 플래그를 반환합니다.
     */
    Asset::ETextureUsage Asset::UTexture2D::GetUsage() const {
        return mUsage;
    }

    /**
     * @brief 현재 형식·색 공간과 호환되는 GPU 사용 용도 플래그를 설정합니다.
     */
    bool Asset::UTexture2D::SetUsage(Asset::ETextureUsage Usage) {
        if (!ValidateUsage(mFormat, Usage, mSRGB) || (mExternalResource && mUsage != Usage)) {
            return false;
        }

        if (mUsage != Usage) {
            mUsage = Usage;

            ReleaseResources();
        }

        return true;
    }

    /**
     * @brief 크기·형식·용도와 픽셀 또는 연결된 리소스 구성이 유효한지 검사합니다.
     */
    bool Asset::UTexture2D::IsValid() const {
        return IsValidSize(mSizeX, mSizeY) && ValidateUsage(mFormat, mUsage, mSRGB);
    }

    /**
     * @brief 현재 CPU 데이터를 사용하여 Direct3D 리소스를 생성합니다.
     */
    bool Asset::UTexture2D::CreateResources(ID3D11Device* Device) {
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
        Description.Usage = !mPixels.empty() && (mUsage & (Asset::ETextureUsage::RenderTarget | Asset::ETextureUsage::DepthStencil)) == Asset::ETextureUsage::None ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
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

    /**
     * @brief 외부 Direct3D 텍스처에 COM 참조를 유지하고 지정 용도의 뷰를 생성합니다.
     */
    bool Asset::UTexture2D::AttachResource(ID3D11Texture2D* Texture, Asset::ETextureUsage Usage) {
        if (Texture == nullptr) {
            return false;
        }

        if (Texture == mTexture.Get() && Usage == mUsage && HasResources()) {
            return true;
        }

        D3D11_TEXTURE2D_DESC Description{};

        Texture->GetDesc(&Description);

        Asset::ETextureFormat Format{};
        bool SRGB{};

        switch (Description.Format) {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            Format = Asset::ETextureFormat::RGBA8;
            SRGB = Description.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

            break;

        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            Format = Asset::ETextureFormat::BGRA8;
            SRGB = Description.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

            break;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            Format = Asset::ETextureFormat::RGBA16Float;

            break;

        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            Format = Asset::ETextureFormat::RGBA32Float;

            break;

        case DXGI_FORMAT_R32_FLOAT:
            Format = Asset::ETextureFormat::R32Float;

            break;

        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            Format = Asset::ETextureFormat::D24S8;

            break;

        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
            Format = Asset::ETextureFormat::D32Float;

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

    /**
     * @brief 소유한 GPU 리소스와 뷰를 해제하고 메모리 집계를 초기화합니다.
     */
    void Asset::UTexture2D::ReleaseResources() {
        mDepthStencilView.Reset();
        mRenderTargetView.Reset();
        mShaderResourceView.Reset();
        mTexture.Reset();
        mResourceBytes = 0;
    }

    /**
     * @brief 사용할 수 있는 GPU 리소스를 보유하는지 검사합니다.
     */
    bool Asset::UTexture2D::HasResources() const {
        return mTexture != nullptr && ((mUsage & Asset::ETextureUsage::ShaderResource) == Asset::ETextureUsage::None || mShaderResourceView != nullptr) && ((mUsage & Asset::ETextureUsage::RenderTarget) == Asset::ETextureUsage::None || mRenderTargetView != nullptr) && ((mUsage & Asset::ETextureUsage::DepthStencil) == Asset::ETextureUsage::None || mDepthStencilView != nullptr);
    }

    /**
     * @brief Direct3D 2차원 텍스처의 비소유 주소를 반환합니다.
     */
    ID3D11Texture2D* Asset::UTexture2D::GetTexture() const {
        return mTexture.Get();
    }

    /**
     * @brief 셰이더에서 읽을 리소스 뷰의 비소유 주소를 반환합니다.
     */
    ID3D11ShaderResourceView* Asset::UTexture2D::GetShaderResourceView() const {
        return mShaderResourceView.Get();
    }

    /**
     * @brief 렌더 타깃 출력 뷰의 비소유 주소를 반환합니다.
     */
    ID3D11RenderTargetView* Asset::UTexture2D::GetRenderTargetView() const {
        return mRenderTargetView.Get();
    }

    /**
     * @brief 깊이·스텐실 출력 뷰의 비소유 주소를 반환합니다.
     */
    ID3D11DepthStencilView* Asset::UTexture2D::GetDepthStencilView() const {
        return mDepthStencilView.Get();
    }

    /**
     * @brief 현재 보유한 GPU 리소스의 추정 바이트 수를 반환합니다.
     */
    std::size_t Asset::UTexture2D::GetResourceBytes() const {
        return mResourceBytes;
    }

    /**
     * @brief DirectX 이미지에서 지원 형식의 픽셀과 DDS 데이터를 구성합니다.
     */
    bool Asset::UTexture2D::SetImageData(DirectX::ScratchImage& SourceImage, bool SRGB) {
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

        Asset::ETextureFormat Format{Asset::ETextureFormat::RGBA8};

        switch (SourceImage.GetMetadata().format) {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            Format = Asset::ETextureFormat::BGRA8;

            break;

        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            Format = Asset::ETextureFormat::RGBA16Float;

            break;

        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            Format = Asset::ETextureFormat::RGBA32Float;

            break;

        case DXGI_FORMAT_R32_FLOAT:
            Format = Asset::ETextureFormat::R32Float;

            break;

        default:
            break;
        }

        SRGB = SRGB && (Format == Asset::ETextureFormat::RGBA8 || Format == Asset::ETextureFormat::BGRA8);

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

    /**
     * @brief 가로·세로 크기가 Direct3D 텍스처의 허용 범위에 있는지 검사합니다.
     */
    bool Asset::UTexture2D::IsValidSize(Core::Uint32 SizeX, Core::Uint32 SizeY) {
        return SizeX > 0 && SizeY > 0 && SizeX <= D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION && SizeY <= D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    }

    /**
     * @brief 지정한 형식이 깊이 또는 깊이·스텐실 형식인지 검사합니다.
     */
    bool Asset::UTexture2D::IsDepthFormat(Asset::ETextureFormat Format) {
        return Format == Asset::ETextureFormat::D24S8 || Format == Asset::ETextureFormat::D32Float;
    }

    /**
     * @brief 픽셀 형식·용도 플래그·sRGB 설정의 조합이 지원되는지 검사합니다.
     */
    bool Asset::UTexture2D::ValidateUsage(Asset::ETextureFormat Format, Asset::ETextureUsage Usage, bool SRGB) {
        const Core::Uint8 Flags{static_cast<Core::Uint8>(Usage)};

        if (GetBytesPerPixel(Format) == 0 || (Flags & ~static_cast<Core::Uint8>(Asset::ETextureUsage::ShaderResource | Asset::ETextureUsage::RenderTarget | Asset::ETextureUsage::DepthStencil)) != 0 || (SRGB && Format != Asset::ETextureFormat::RGBA8 && Format != Asset::ETextureFormat::BGRA8)) {
            return false;
        }

        return IsDepthFormat(Format) ? (Usage & Asset::ETextureUsage::RenderTarget) == Asset::ETextureUsage::None : (Usage & Asset::ETextureUsage::DepthStencil) == Asset::ETextureUsage::None;
    }

    /**
     * @brief 텍스처 형식의 픽셀 하나가 차지하는 바이트 수를 반환합니다.
     */
    Core::Uint32 Asset::UTexture2D::GetBytesPerPixel(Asset::ETextureFormat Format) {
        switch (Format) {
        case Asset::ETextureFormat::RGBA8:
        case Asset::ETextureFormat::BGRA8:
        case Asset::ETextureFormat::R32Float:
        case Asset::ETextureFormat::D24S8:
        case Asset::ETextureFormat::D32Float:
            return 4;

        case Asset::ETextureFormat::RGBA16Float:
            return 8;

        case Asset::ETextureFormat::RGBA32Float:
            return 16;

        default:
            return 0;
        }
    }

    /**
     * @brief 텍스처 용도 플래그를 Direct3D 바인딩 플래그로 변환합니다.
     */
    Core::Uint32 Asset::UTexture2D::GetBindFlags(Asset::ETextureUsage Usage) {
        Core::Uint32 Flags{};

        Flags |= (Usage & Asset::ETextureUsage::ShaderResource) != Asset::ETextureUsage::None ? D3D11_BIND_SHADER_RESOURCE : 0;
        Flags |= (Usage & Asset::ETextureUsage::RenderTarget) != Asset::ETextureUsage::None ? D3D11_BIND_RENDER_TARGET : 0;
        Flags |= (Usage & Asset::ETextureUsage::DepthStencil) != Asset::ETextureUsage::None ? D3D11_BIND_DEPTH_STENCIL : 0;

        return Flags;
    }

    /**
     * @brief 픽셀 형식과 색 공간에 대응하는 Direct3D 리소스 형식을 반환합니다.
     */
    DXGI_FORMAT Asset::UTexture2D::GetResourceFormat(Asset::ETextureFormat Format, bool SRGB) {
        switch (Format) {
        case Asset::ETextureFormat::RGBA8:
            return SRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

        case Asset::ETextureFormat::BGRA8:
            return SRGB ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;

        case Asset::ETextureFormat::RGBA16Float:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;

        case Asset::ETextureFormat::RGBA32Float:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;

        case Asset::ETextureFormat::R32Float:
            return DXGI_FORMAT_R32_FLOAT;

        case Asset::ETextureFormat::D24S8:
            return DXGI_FORMAT_R24G8_TYPELESS;

        case Asset::ETextureFormat::D32Float:
            return DXGI_FORMAT_R32_TYPELESS;

        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    /**
     * @brief 픽셀·깊이 형식과 색 공간에 대응하는 셰이더 리소스 뷰 형식을 반환합니다.
     */
    DXGI_FORMAT Asset::UTexture2D::GetShaderResourceFormat(Asset::ETextureFormat Format, bool SRGB) {
        if (Format == Asset::ETextureFormat::D24S8) {
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        }

        if (Format == Asset::ETextureFormat::D32Float) {
            return DXGI_FORMAT_R32_FLOAT;
        }

        return GetResourceFormat(Format, SRGB);
    }

    /**
     * @brief 깊이 형식에 대응하는 Direct3D 깊이·스텐실 뷰 형식을 반환합니다.
     */
    DXGI_FORMAT Asset::UTexture2D::GetDepthStencilFormat(Asset::ETextureFormat Format) {
        if (Format == Asset::ETextureFormat::D24S8) {
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        }

        return Format == Asset::ETextureFormat::D32Float ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_UNKNOWN;
    }

    /**
     * @brief 텍스처 용도에 따라 셰이더 리소스·렌더 타깃·깊이 뷰를 생성합니다.
     */
    bool Asset::UTexture2D::CreateViews(ID3D11Device* Device, ID3D11Texture2D* Texture, Asset::ETextureFormat Format, Asset::ETextureUsage Usage, bool SRGB, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& ShaderResourceView, Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& RenderTargetView, Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& DepthStencilView) {
        if ((Usage & Asset::ETextureUsage::ShaderResource) != Asset::ETextureUsage::None) {
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

        if ((Usage & Asset::ETextureUsage::RenderTarget) != Asset::ETextureUsage::None) {
            D3D11_RENDER_TARGET_VIEW_DESC Description{};
            Description.Format = GetResourceFormat(Format, SRGB);
            Description.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

            if (FAILED(Device->CreateRenderTargetView(Texture, &Description, RenderTargetView.GetAddressOf()))) {
                return false;
            }
        }

        if ((Usage & Asset::ETextureUsage::DepthStencil) != Asset::ETextureUsage::None) {
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
