#include "FMaterialAssetData.h"
#include "Core/FilePath.h"
#include "Engine/FAssetManager.h"
#include "Engine/UMaterial.h"
#include "Serialization/MemoryArchive.h"

FMaterialAssetData::FMaterialAssetData() = default;

FMaterialAssetData::FMaterialAssetData(const UMaterial& Material)
	: mName{Material.GetName()},
	  mShaderPath{Material.GetShaderPath()},
	  mBaseColor{Material.GetBaseColor()},
	  mBaseColorTexturePath{Material.GetBaseColorTexturePath()},
	  mNormalMapTexturePath{Material.GetNormalMapTexturePath()},
	  mSpecularStrength{Material.GetSpecularStrength()},
	  mShininess{Material.GetShininess()},
	  mNormalStrength{Material.GetNormalStrength()},
	  mTwoSided{Material.IsTwoSided()} {
}

void FMaterialAssetData::Serialize(FMemoryArchive& Archive) {
    Uint32 Version{1};
    Archive.Serialize("Version", Version);

    if (Archive.HasError() || Version != 1) {
        Archive.SetError("Unsupported material asset version");
        return;
    }

    Archive.Serialize("Name", mName);
    Archive.Serialize("Shader", mShaderPath);
    Archive.Serialize("BaseColor", mBaseColor);
    Archive.Serialize("BaseColorTexture", mBaseColorTexturePath);
    Archive.Serialize("NormalMap", mNormalMapTexturePath);
    Archive.Serialize("SpecularStrength", mSpecularStrength);
    Archive.Serialize("Shininess", mShininess);
    Archive.Serialize("NormalStrength", mNormalStrength);
    Archive.Serialize("TwoSided", mTwoSided);
}

bool FMaterialAssetData::ApplyTo(UMaterial& Material, FString& Error) const {
    if (!Material.SetName(mName) || !Material.SetShaderPath(mShaderPath) || !Material.SetBaseColor(mBaseColor) || !Material.SetBaseColorTexturePath(mBaseColorTexturePath) || !Material.SetNormalMapTexturePath(mNormalMapTexturePath) || !Material.SetSpecularStrength(mSpecularStrength) || !Material.SetShininess(mShininess) || !Material.SetNormalStrength(mNormalStrength) || !Material.SetTwoSided(mTwoSided)) {
        Error = "Invalid material asset settings";
        return false;
    }

    return true;
}

bool FMaterialAssetData::ValidateReferences(const FAssetManager& Assets, FString& Error) const {
    if (mName.empty() || mName.size() > 255 || mName.find('\0') != FString::npos) {
        Error = "Invalid material asset name";
        return false;
    }

    for (const FString* Reference : {&mShaderPath, &mBaseColorTexturePath, &mNormalMapTexturePath}) {
        if (!Reference->empty() && Assets.ResolvePath(FilePath::FromUtf8(*Reference), Error).empty()) {
            return false;
        }
    }

    return true;
}
