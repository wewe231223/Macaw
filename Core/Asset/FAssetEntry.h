#pragma once

#include "FAssetHandle.h"
#include "FAssetPath.h"
#include "UAsset.h"

#include <filesystem>
#include <memory>

enum class EAssetType : Uint8 {
    Texture,
    Pipeline,
    Material,
    Mesh,
    Font,
    END
};

struct FTextureMetadata {
    bool mMakeDDS{true};
    bool mGenerateMipMap{true};
};

struct FMeshMetadata {
    bool mFlipUV{false};
};

struct FAssetEntry {
    FAssetPath mAssetPath{};
    std::filesystem::path mPhysicalPath{};
    std::filesystem::path mSidecarPath{};
    FGuid mPersistentGuid{};
    EAssetType mAssetType{EAssetType::END};
    FAssetHandle mHandle{};
    std::unique_ptr<UAsset> mAsset{};
    FTextureMetadata mTextureMetadata{};
    FMeshMetadata mMeshMetadata{};
};
