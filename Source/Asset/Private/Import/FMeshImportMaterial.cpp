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

    /**
     * @brief 전달받은 설정을 보관하도록 FMeshImportMaterial 객체를 초기화합니다.
     */
    Asset::FMeshImportMaterial::FMeshImportMaterial(Core::FString Name, const Core::FVector4& BaseColor, bool TwoSided, Asset::FMeshImportTexture BaseColorTexture, Asset::FMeshImportTexture NormalMapTexture)
		: mName{std::move(Name)},
		  mBaseColor{BaseColor},
		  mTwoSided{TwoSided},
		  mBaseColorTexture{std::move(BaseColorTexture)},
		  mNormalMapTexture{std::move(NormalMapTexture)} {
    }

    /**
     * @brief 임포트한 원본 재질 이름을 반환합니다.
     */
    const Core::FString& Asset::FMeshImportMaterial::GetName() const {
        return mName;
    }

    /**
     * @brief 원본 재질에서 가져온 RGBA 기본색을 반환합니다.
     */
    const Core::FVector4& Asset::FMeshImportMaterial::GetBaseColor() const {
        return mBaseColor;
    }

    /**
     * @brief 원본 재질의 양면 렌더링 설정을 반환합니다.
     */
    bool Asset::FMeshImportMaterial::IsTwoSided() const {
        return mTwoSided;
    }

    /**
     * @brief 원본 재질의 기본색 텍스처 참조를 반환합니다.
     */
    const Asset::FMeshImportTexture& Asset::FMeshImportMaterial::GetBaseColorTexture() const {
        return mBaseColorTexture;
    }

    /**
     * @brief 원본 재질의 노멀 맵 참조를 반환합니다.
     */
    const Asset::FMeshImportTexture& Asset::FMeshImportMaterial::GetNormalMapTexture() const {
        return mNormalMapTexture;
    }

    /**
     * @brief 정반사 색상 값을 반환합니다.
     */
    const Core::FVector4& Asset::FMeshImportMaterial::GetSpecularColor() const {
        return mSpecularColor;
    }

    /**
     * @brief 주변광 색상 값을 반환합니다.
     */
    const Core::FVector4& Asset::FMeshImportMaterial::GetAmbientColor() const {
        return mAmbientColor;
    }

    /**
     * @brief 자체 발광 색상 값을 반환합니다.
     */
    const Core::FVector4& Asset::FMeshImportMaterial::GetEmissiveColor() const {
        return mEmissiveColor;
    }

    /**
     * @brief 환경 반사 색상 값을 반환합니다.
     */
    const Core::FVector4& Asset::FMeshImportMaterial::GetReflectionColor() const {
        return mReflectionColor;
    }

    /**
     * @brief 정반사 세기 값을 반환합니다.
     */
    float Asset::FMeshImportMaterial::GetSpecularStrength() const {
        return mSpecularStrength;
    }

    /**
     * @brief 정반사 지수 값을 반환합니다.
     */
    float Asset::FMeshImportMaterial::GetShininess() const {
        return mShininess;
    }

    /**
     * @brief 환경 반사 세기 값을 반환합니다.
     */
    float Asset::FMeshImportMaterial::GetReflectionStrength() const {
        return mReflectionStrength;
    }

    /**
     * @brief 정반사 텍스처 값을 반환합니다.
     */
    const Asset::FMeshImportTexture& Asset::FMeshImportMaterial::GetSpecularTexture() const {
        return mSpecularTexture;
    }

    /**
     * @brief 자체 발광 텍스처 값을 반환합니다.
     */
    const Asset::FMeshImportTexture& Asset::FMeshImportMaterial::GetEmissiveTexture() const {
        return mEmissiveTexture;
    }

    /**
     * @brief 환경 반사 텍스처 값을 반환합니다.
     */
    const Asset::FMeshImportTexture& Asset::FMeshImportMaterial::GetReflectionTexture() const {
        return mReflectionTexture;
    }

    /**
     * @brief 조명 색상·세기·정반사 지수를 검증하고 임포트한 조명 텍스처 참조와 함께 저장합니다.
     */
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
