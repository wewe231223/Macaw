#include "pch.h"
#include "Asset/FSurfaceParameters.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "Asset/FAssetRegistry.h"

FAssetHandle FMaterialTextureMap::GetTexture() const {
    return mTexture.GetHandle();
}

void FMaterialTextureMap::SetTexture(FAssetHandle Texture) {
    mTexture.Assign(Texture, FAssetRegistry::TryGet());
}

void FMaterialTextureMap::SetTextureReference(const FAssetReference& Texture) {
    mTexture = Texture;
}

void FMaterialTextureMap::Resolve(const IAssetResolver* Resolver) {
    mTexture.Resolve(Resolver);
}

void FMaterialTextureMap::Serialize(FArchive& Archive) {
    SerializeAssetReference(Archive, "TextureGuid", "TexturePath", mTexture);
}
