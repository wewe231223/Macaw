/**
 * @file FTextureAssetData.cpp
 * @brief 텍스처 에셋의 바이너리 직렬화와 이미지 복원을 구현합니다.
 * @ingroup Asset
 */

#include "FTextureAssetData.h"
#include "Asset/UTexture2D.h"
#include "Serialization/MemoryArchive.h"

#include <utility>

namespace Asset {

    Asset::FTextureAssetData::FTextureAssetData(const Asset::UTexture2D& Texture)
		: mWidth{Texture.GetSizeX()},
		  mHeight{Texture.GetSizeY()},
		  mFormat{static_cast<Core::Uint32>(Texture.GetFormat())},
		  mSRGB{Texture.IsSRGB()},
		  mDDS{!Texture.GetDDSData().empty()} {
        const auto& Data{mDDS ? Texture.GetDDSData() : Texture.GetPixels()};

        if (!Data.empty()) {
            mImageData.assign(reinterpret_cast<const char*>(Data.data()), Data.size());
        }
    }

    void Asset::FTextureAssetData::Serialize(Serialization::FMemoryArchive& Archive) {
        Archive.Serialize("Width", mWidth);
        Archive.Serialize("Height", mHeight);
        Archive.Serialize("Format", mFormat);
        Archive.Serialize("SRGB", mSRGB);
        Archive.Serialize("DDS", mDDS);
        Archive.Serialize("ImageData", mImageData);
    }

    bool Asset::FTextureAssetData::ApplyTo(Asset::UTexture2D& Texture, Core::FString& Error) const {
        Error.clear();

        if (mFormat >= static_cast<Core::Uint32>(Asset::ETextureFormat::D24S8)) {
            Error = "Invalid texture asset data";
            return false;
        }

        Core::TArray<Core::Uint8> Bytes{mImageData.begin(), mImageData.end()};
        const bool Loaded{mDDS ? Texture.SetDDSData(Bytes, mSRGB) : Texture.SetPixels(mWidth, mHeight, static_cast<Asset::ETextureFormat>(mFormat), std::move(Bytes)) && Texture.SetSRGB(mSRGB)};

        if (!Loaded || Texture.GetSizeX() != mWidth || Texture.GetSizeY() != mHeight || Texture.GetFormat() != static_cast<Asset::ETextureFormat>(mFormat)) {
            Error = "Invalid texture image data";
            return false;
        }

        return true;
    }

}
