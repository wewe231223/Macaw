#include "pch.h"
#include "Asset/UTexture.h"
#include "Core/Base/ErrorHandler.h"

#include <cctype>
#include <DirectXTex.h>

namespace {
    bool GetTextureExtension(const std::filesystem::path& Path, ETextureExtension& OutExtension) {
        FString Extension{Path.extension().generic_string().c_str()};
        std::ranges::transform(Extension, Extension.begin(), [](unsigned char Character) {
            return static_cast<char>(std::tolower(Character));
        });

        if (Extension == ".dds") {
            OutExtension = ETextureExtension::DDS;
            return true;
        }

        if (Extension == ".tga") {
            OutExtension = ETextureExtension::TGA;
            return true;
        }

        if (Extension == ".bmp") {
            OutExtension = ETextureExtension::BMP;
            return true;
        }

        if (Extension == ".png") {
            OutExtension = ETextureExtension::PNG;
            return true;
        }

        if (Extension == ".gif") {
            OutExtension = ETextureExtension::GIF;
            return true;
        }

        if (Extension == ".tif") {
            OutExtension = ETextureExtension::TIF;
            return true;
        }

        if (Extension == ".tiff") {
            OutExtension = ETextureExtension::TIFF;
            return true;
        }

        if (Extension == ".jpg") {
            OutExtension = ETextureExtension::JPG;
            return true;
        }

        if (Extension == ".jpeg") {
            OutExtension = ETextureExtension::JPEG;
            return true;
        }

        if (Extension == ".hdr") {
            OutExtension = ETextureExtension::HDR;
            return true;
        }

        return false;
    }

    DXGI_FORMAT GetDXGIFormat(ETextureFormat TextureFormat) {
        switch (TextureFormat) {
            case ETextureFormat::UNORM:
                return DXGI_FORMAT_R8G8B8A8_UNORM;

            case ETextureFormat::SRGB:
                return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

            default:
                return DXGI_FORMAT_UNKNOWN;
        }
    }
}

bool UTexture::Initialize(const std::filesystem::path& ImagePath, bool MakeDDS, ETextureFormat TextureFormat, bool BGenerateMipMap) {
    ETextureExtension TextureExtension{};
    const bool BValidExtension{GetTextureExtension(ImagePath, TextureExtension)};

    ErrorHandler::Report(!BValidExtension, "[ UTexture ]", "Unsupported texture extension: " + ImagePath.string(), ErrorHandler::EErrorLevel::Critical);

    if (!BValidExtension) {
        return false;
    }

    return InitializeInternal(ImagePath, TextureFormat, TextureExtension, BGenerateMipMap, MakeDDS);
}

bool UTexture::InitializeInternal(const std::filesystem::path& ImagePath, ETextureFormat TextureFormat, ETextureExtension TextureExtension, bool BGenerateMipMap, bool MakeDDS) {
    if (!UAsset::Initialize(ImagePath)) {
        ErrorHandler::Report("[ UTexture ]", "Texture image path is invalid: " + ImagePath.string(), ErrorHandler::EErrorLevel::Critical);
        return false;
    }

    const std::filesystem::path& Path{ImagePath};
    const DXGI_FORMAT TargetFormat{GetDXGIFormat(TextureFormat)};

    if (TargetFormat == DXGI_FORMAT_UNKNOWN) {
        ErrorHandler::Report("[ UTexture ]", "Unsupported texture format setting: " + Path.string(), ErrorHandler::EErrorLevel::Critical);
        return false;
    }

    DirectX::ScratchImage SourceImage{};
    DirectX::ScratchImage ConvertedImage{};
    DirectX::ScratchImage GeneratedMipChain{};
    DirectX::TexMetadata SourceImageMetaData{};
    HRESULT Result{S_OK};

    if (TextureExtension == ETextureExtension::DDS) {
        Result = DirectX::LoadFromDDSFile(Path.wstring().c_str(), DirectX::DDS_FLAGS_NONE, &SourceImageMetaData, SourceImage);
        ErrorHandler::ReportHRESULT(Result, "[ UTexture ]", "Failed to load DDS texture: " + Path.string(), ErrorHandler::EErrorLevel::Critical);
    } else if (TextureExtension == ETextureExtension::TGA) {
        Result = DirectX::LoadFromTGAFile(Path.wstring().c_str(), DirectX::TGA_FLAGS_NONE, &SourceImageMetaData, SourceImage);
        ErrorHandler::ReportHRESULT(Result, "[ UTexture ]", "Failed to load TGA texture: " + Path.string(), ErrorHandler::EErrorLevel::Critical);
    } else if (TextureExtension == ETextureExtension::BMP || TextureExtension == ETextureExtension::PNG || TextureExtension == ETextureExtension::GIF || TextureExtension == ETextureExtension::TIF || TextureExtension == ETextureExtension::TIFF || TextureExtension == ETextureExtension::JPG || TextureExtension == ETextureExtension::JPEG) {
        Result = DirectX::LoadFromWICFile(Path.wstring().c_str(), DirectX::WIC_FLAGS_NONE, &SourceImageMetaData, SourceImage);
        ErrorHandler::ReportHRESULT(Result, "[ UTexture ]", "Failed to load WIC texture: " + Path.string(), ErrorHandler::EErrorLevel::Critical);
    } else if (TextureExtension == ETextureExtension::HDR) {
        Result = DirectX::LoadFromHDRFile(Path.wstring().c_str(), &SourceImageMetaData, SourceImage);
        ErrorHandler::ReportHRESULT(Result, "[ UTexture ]", "Failed to load HDR texture: " + Path.string(), ErrorHandler::EErrorLevel::Critical);
    } else {
        ErrorHandler::Report("[ UTexture ]", "Unsupported texture extension setting: " + Path.string(), ErrorHandler::EErrorLevel::Critical);

        return false;
    }

    if (FAILED(Result) || SourceImage.GetImageCount() == 0) {
        return false;
    }

    if (MakeDDS && TextureExtension != ETextureExtension::DDS && SourceImage.GetMetadata().format != TargetFormat) {
        Result = DirectX::Convert(SourceImage.GetImages(), SourceImage.GetImageCount(), SourceImage.GetMetadata(), TargetFormat, DirectX::TEX_FILTER_FANT, DirectX::TEX_THRESHOLD_DEFAULT, ConvertedImage);

        if (FAILED(Result)) {
            return false;
        }

        SourceImage = std::move(ConvertedImage);
    }

    if (MakeDDS && BGenerateMipMap && SourceImage.GetMetadata().mipLevels == 1) {
        Result = DirectX::GenerateMipMaps(SourceImage.GetImages(), SourceImage.GetImageCount(), SourceImage.GetMetadata(), DirectX::TEX_FILTER_FANT, 0, GeneratedMipChain);

        if (FAILED(Result)) {
            return false;
        }

        SourceImage = std::move(GeneratedMipChain);
    }

    mSourceImage = std::make_unique<DirectX::ScratchImage>(std::move(SourceImage));
    ++mRenderRevision;

    return true;
}

void UTexture::Serialize(FArchive& Ar) {
    UAsset::Serialize(Ar);
}

UTexture::UTexture() {
}

UTexture::~UTexture() {
}

const DirectX::ScratchImage* UTexture::GetSourceImage() const {
    return mSourceImage.get();
}

Uint64 UTexture::GetRenderRevision() const {
    return mRenderRevision;
}
