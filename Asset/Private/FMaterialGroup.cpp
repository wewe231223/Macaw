#include "pch.h"
#include "Asset/FMaterialGroup.h"

void FMaterialTextureMap::SetTexture(FAssetHandle Texture) {
    mTexture = Texture;
    mSourcePath.clear();
}
