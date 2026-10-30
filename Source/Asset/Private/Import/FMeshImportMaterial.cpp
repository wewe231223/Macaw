/**
 * @file FMeshImportMaterial.cpp
 * @brief 메시 임포트에서 읽은 재질 이름·기본색·양면 설정과 텍스처 참조입니다.
 * @ingroup Asset
 */

#include "Asset/Import/FMeshImportMaterial.h"

#include <utility>

namespace Asset {

    Asset::FMeshImportMaterial::FMeshImportMaterial(Core::FString Name, const Core::FVector4& BaseColor, bool TwoSided, Asset::FMeshImportTexture BaseColorTexture, Asset::FMeshImportTexture NormalMapTexture)
		: mName{std::move(Name)},
		  mBaseColor{BaseColor},
		  mTwoSided{TwoSided},
		  mBaseColorTexture{std::move(BaseColorTexture)},
		  mNormalMapTexture{std::move(NormalMapTexture)} {
    }

    const Core::FString& Asset::FMeshImportMaterial::GetName() const {
        return mName;
    }

    const Core::FVector4& Asset::FMeshImportMaterial::GetBaseColor() const {
        return mBaseColor;
    }

    bool Asset::FMeshImportMaterial::IsTwoSided() const {
        return mTwoSided;
    }

    const Asset::FMeshImportTexture& Asset::FMeshImportMaterial::GetBaseColorTexture() const {
        return mBaseColorTexture;
    }

    const Asset::FMeshImportTexture& Asset::FMeshImportMaterial::GetNormalMapTexture() const {
        return mNormalMapTexture;
    }

}
