/**
 * @file FMeshImportMaterial.cpp
 * @brief 메시 임포트에서 읽은 재질 이름·기본색·양면 설정과 텍스처 참조입니다.
 * @ingroup Asset
 */

#include "Asset/Import/FMeshImportMaterial.h"

#include <algorithm>
#include <array>
#include <cmath>
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

    const Core::FVector4& Asset::FMeshImportMaterial::GetSpecularColor() const {
        return mSpecularColor;
    }

    const Core::FVector4& Asset::FMeshImportMaterial::GetAmbientColor() const {
        return mAmbientColor;
    }

    const Core::FVector4& Asset::FMeshImportMaterial::GetEmissiveColor() const {
        return mEmissiveColor;
    }

    const Core::FVector4& Asset::FMeshImportMaterial::GetReflectionColor() const {
        return mReflectionColor;
    }

    float Asset::FMeshImportMaterial::GetSpecularStrength() const {
        return mSpecularStrength;
    }

    float Asset::FMeshImportMaterial::GetShininess() const {
        return mShininess;
    }

    float Asset::FMeshImportMaterial::GetReflectionStrength() const {
        return mReflectionStrength;
    }

    const Asset::FMeshImportTexture& Asset::FMeshImportMaterial::GetSpecularTexture() const {
        return mSpecularTexture;
    }

    const Asset::FMeshImportTexture& Asset::FMeshImportMaterial::GetEmissiveTexture() const {
        return mEmissiveTexture;
    }

    const Asset::FMeshImportTexture& Asset::FMeshImportMaterial::GetReflectionTexture() const {
        return mReflectionTexture;
    }

    bool Asset::FMeshImportMaterial::SetLighting(const Core::FVector4& SpecularColor, float SpecularStrength, float Shininess, const Core::FVector4& AmbientColor, const Core::FVector4& EmissiveColor, const Core::FVector4& ReflectionColor, float ReflectionStrength, Asset::FMeshImportTexture SpecularTexture, Asset::FMeshImportTexture EmissiveTexture, Asset::FMeshImportTexture ReflectionTexture) {
        const std::array<Core::FVector4, 4> Colors{SpecularColor, AmbientColor, EmissiveColor, ReflectionColor};

        if (!std::all_of(Colors.begin(), Colors.end(), [](const Core::FVector4& Color) {
            return Color.IsFinite() && Color.x >= 0.0f && Color.y >= 0.0f && Color.z >= 0.0f && Color.w >= 0.0f && Color.w <= 1.0f;
        }) || !std::isfinite(SpecularStrength) || SpecularStrength < 0.0f || SpecularStrength > 1.0f || !std::isfinite(Shininess) || Shininess < 0.0f || !std::isfinite(ReflectionStrength) || ReflectionStrength < 0.0f || ReflectionStrength > 1.0f) {
            return false;
        }

        mSpecularColor = SpecularColor;
        mAmbientColor = AmbientColor;
        mEmissiveColor = EmissiveColor;
        mReflectionColor = ReflectionColor;
        mSpecularStrength = SpecularStrength;
        mShininess = std::clamp(Shininess, 1.0f, 1024.0f);
        mReflectionStrength = ReflectionStrength;
        mSpecularTexture = std::move(SpecularTexture);
        mEmissiveTexture = std::move(EmissiveTexture);
        mReflectionTexture = std::move(ReflectionTexture);
        return true;
    }

}
