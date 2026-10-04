#pragma once
#include "Asset/UAsset.h"
#include <memory>

namespace DirectX {
    class ScratchImage;
}

enum class ETextureFormat : Uint8 {
    UNORM,
    SRGB
};

enum class ETextureExtension : Uint8 {
    DDS,
    TGA,
    BMP,
    PNG,
    GIF,
    TIF,
    TIFF,
    JPG,
    JPEG,
    HDR
};

class UTexture : public UAsset {
public:
    UTexture();
    ~UTexture() override;

    UTexture(const UTexture&) = delete;
    UTexture& operator=(const UTexture&) = delete;

    UTexture(UTexture&&) noexcept = delete;
    UTexture& operator=(UTexture&&) noexcept = delete;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UTexture, UAsset);

    bool Initialize(const std::filesystem::path& ImagePath, bool MakeDDS, ETextureFormat TextureFormat = ETextureFormat::UNORM, bool BGenerateMipMap = true);

    const DirectX::ScratchImage* GetSourceImage() const;
    Uint64 GetRenderRevision() const;

private:
    void Serialize(FArchive& Ar) override;
    bool InitializeInternal(const std::filesystem::path& ImagePath, ETextureFormat TextureFormat, ETextureExtension TextureExtension, bool BGenerateMipMap, bool MakeDDS);

private:
    std::unique_ptr<DirectX::ScratchImage> mSourceImage{};
    Uint64 mRenderRevision{1};
};
