#include "Engine/Import/FMeshImportMaterial.h"

#include <utility>

FMeshImportMaterial::FMeshImportMaterial(FString Name, const FVector4& BaseColor, bool TwoSided, FMeshImportTexture BaseColorTexture, FMeshImportTexture NormalMapTexture)
	: mName{std::move(Name)},
	  mBaseColor{BaseColor},
	  mTwoSided{TwoSided},
	  mBaseColorTexture{std::move(BaseColorTexture)},
	  mNormalMapTexture{std::move(NormalMapTexture)} {
}

const FString& FMeshImportMaterial::GetName() const {
    return mName;
}

const FVector4& FMeshImportMaterial::GetBaseColor() const {
    return mBaseColor;
}

bool FMeshImportMaterial::IsTwoSided() const {
    return mTwoSided;
}

const FMeshImportTexture& FMeshImportMaterial::GetBaseColorTexture() const {
    return mBaseColorTexture;
}

const FMeshImportTexture& FMeshImportMaterial::GetNormalMapTexture() const {
    return mNormalMapTexture;
}
