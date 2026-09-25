#include "pch.h"
#include "FMaterialChunkSignature.h"

bool FMaterialChunkSignature::IsValid() const {
    return mTextureFieldCount <= MaxMaterialTextureFields;
}

FAssetHandle FMaterialChunkSignature::GetTextureHandle(Uint8 TextureFieldIndex) const {
    if (TextureFieldIndex >= mTextureFieldCount) {
        return {};
    }

    return mTextureHandles[TextureFieldIndex];
}

std::size_t FMaterialChunkSignature::GetHash() const noexcept {
    std::size_t Hash{std::hash<Uint8>{}(mTextureFieldCount)};

    for (Uint8 TextureFieldIndex{}; TextureFieldIndex < mTextureFieldCount; ++TextureFieldIndex) {
        const FAssetHandle Handle{mTextureHandles[TextureFieldIndex]};
        Hash ^= std::hash<Uint32>{}(Handle.mId) + static_cast<std::size_t>(0x9e3779b9u) + (Hash << 6) + (Hash >> 2);
        Hash ^= std::hash<Uint32>{}(Handle.mGeneration) + static_cast<std::size_t>(0x9e3779b9u) + (Hash << 6) + (Hash >> 2);
    }

    return Hash;
}

bool FMaterialChunkSignatureBuilder::AddTexture(FAssetHandle TextureHandle) {
    if (mSignature.mTextureFieldCount >= MaxMaterialTextureFields) {
        return false;
    }

    mSignature.mTextureHandles[mSignature.mTextureFieldCount] = TextureHandle;
    ++mSignature.mTextureFieldCount;

    return true;
}

FMaterialChunkSignature FMaterialChunkSignatureBuilder::Build() const {
    return mSignature;
}

void FMaterialChunkSignatureBuilder::Reset() {
    mSignature = {};
}

std::size_t std::hash<FMaterialChunkSignature>::operator()(const FMaterialChunkSignature& Signature) const noexcept {
    return Signature.GetHash();
}
