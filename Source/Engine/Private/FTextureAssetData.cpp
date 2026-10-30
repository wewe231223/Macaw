/**
 * @file FTextureAssetData.cpp
 * @brief 텍스처 에셋의 바이너리 직렬화와 이미지 복원을 구현합니다.
 * @ingroup Engine
 */

#include "FTextureAssetData.h"
#include "Engine/UTexture2D.h"
#include "Serialization/MemoryArchive.h"

#include <utility>

FTextureAssetData::FTextureAssetData(const UTexture2D& Texture)
	: mWidth{Texture.GetSizeX()},
	  mHeight{Texture.GetSizeY()},
	  mFormat{static_cast<Uint32>(Texture.GetFormat())},
	  mSRGB{Texture.IsSRGB()},
	  mDDS{!Texture.GetDDSData().empty()} {
    const auto& Data{mDDS ? Texture.GetDDSData() : Texture.GetPixels()};

    if (!Data.empty()) {
        mImageData.assign(reinterpret_cast<const char*>(Data.data()), Data.size());
    }
}

void FTextureAssetData::Serialize(FMemoryArchive& Archive) {
    Archive.Serialize("Width", mWidth);
    Archive.Serialize("Height", mHeight);
    Archive.Serialize("Format", mFormat);
    Archive.Serialize("SRGB", mSRGB);
    Archive.Serialize("DDS", mDDS);
    Archive.Serialize("ImageData", mImageData);
}

bool FTextureAssetData::ApplyTo(UTexture2D& Texture, FString& Error) const {
    Error.clear();

    if (mFormat >= static_cast<Uint32>(ETextureFormat::D24S8)) {
        Error = "Invalid texture asset data";
        return false;
    }

    TArray<Uint8> Bytes{mImageData.begin(), mImageData.end()};
    const bool Loaded{mDDS ? Texture.SetDDSData(Bytes, mSRGB) : Texture.SetPixels(mWidth, mHeight, static_cast<ETextureFormat>(mFormat), std::move(Bytes)) && Texture.SetSRGB(mSRGB)};

    if (!Loaded || Texture.GetSizeX() != mWidth || Texture.GetSizeY() != mHeight || Texture.GetFormat() != static_cast<ETextureFormat>(mFormat)) {
        Error = "Invalid texture image data";
        return false;
    }

    return true;
}
