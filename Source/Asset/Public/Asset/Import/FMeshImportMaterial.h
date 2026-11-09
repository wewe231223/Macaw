/**
 * @file FMeshImportMaterial.h
 * @brief 메시 임포트에서 읽은 재질 이름·기본색·양면 설정과 텍스처 참조입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Math/FVector4.h"
#include "Asset/Import/FMeshImportTexture.h"

namespace Asset {

    /**
     * @brief 메시 임포트에서 읽은 재질 이름·기본색·양면 설정과 텍스처 참조입니다.
     */
    class FMeshImportMaterial {
    public:
        /**
         * @brief 기본 설정으로 FMeshImportMaterial 객체를 초기화합니다.
         */
        FMeshImportMaterial() = default;

        /**
         * @brief 전달받은 설정을 보관하도록 FMeshImportMaterial 객체를 초기화합니다.
         * @param[in] Name 조회하거나 지정할 이름입니다.
         * @param[in] BaseColor 원본 재질의 기본 RGBA 색상입니다.
         * @param[in] TwoSided 원본 재질의 양면 렌더링 설정입니다.
         * @param[in] BaseColorTexture 원본 재질의 기본색 텍스처 참조입니다.
         * @param[in] NormalMapTexture 원본 재질의 노멀 맵 참조입니다.
         */
        FMeshImportMaterial(Core::FString Name, const Core::FVector4& BaseColor, bool TwoSided, Asset::FMeshImportTexture BaseColorTexture, Asset::FMeshImportTexture NormalMapTexture);

    public:
        /**
         * @brief 임포트한 원본 재질 이름을 반환합니다.
         * @return 소유 객체가 보관하는 이름의 읽기 전용 문자열 참조입니다.
         */
        const Core::FString& GetName() const;

        /**
         * @brief 원본 재질에서 가져온 RGBA 기본색을 반환합니다.
         * @return 보관 중인 기본색의 읽기 전용 참조입니다.
         */
        const Core::FVector4& GetBaseColor() const;

        /**
         * @brief 원본 재질의 양면 렌더링 설정을 반환합니다.
         * @return 뒷면 제거를 끄는 재질이면 true입니다.
         */
        bool IsTwoSided() const;

        /**
         * @brief 원본 재질의 기본색 텍스처 참조를 반환합니다.
         * @return 텍스처 출처와 경로·내장 참조를 가진 읽기 전용 값 객체 참조입니다.
         */
        const Asset::FMeshImportTexture& GetBaseColorTexture() const;

        /**
         * @brief 원본 재질의 노멀 맵 참조를 반환합니다.
         * @return 텍스처 출처와 경로·내장 참조를 가진 읽기 전용 값 객체 참조입니다.
         */
        const Asset::FMeshImportTexture& GetNormalMapTexture() const;

        /**
         * @brief 정반사 색상 값을 반환합니다.
         * @return 저장한 정반사 색상 값 또는 참조입니다.
         */
        const Core::FVector4& GetSpecularColor() const;

        /**
         * @brief 주변광 색상 값을 반환합니다.
         * @return 저장한 주변광 색상 값 또는 참조입니다.
         */
        const Core::FVector4& GetAmbientColor() const;

        /**
         * @brief 자체 발광 색상 값을 반환합니다.
         * @return 저장한 자체 발광 색상 값 또는 참조입니다.
         */
        const Core::FVector4& GetEmissiveColor() const;

        /**
         * @brief 환경 반사 색상 값을 반환합니다.
         * @return 저장한 환경 반사 색상 값 또는 참조입니다.
         */
        const Core::FVector4& GetReflectionColor() const;

        /**
         * @brief 정반사 세기 값을 반환합니다.
         * @return 저장한 정반사 세기 값 또는 참조입니다.
         */
        float GetSpecularStrength() const;

        /**
         * @brief 정반사 지수 값을 반환합니다.
         * @return 저장한 정반사 지수 값 또는 참조입니다.
         */
        float GetShininess() const;

        /**
         * @brief 환경 반사 세기 값을 반환합니다.
         * @return 저장한 환경 반사 세기 값 또는 참조입니다.
         */
        float GetReflectionStrength() const;

        /**
         * @brief 정반사 텍스처 값을 반환합니다.
         * @return 저장한 정반사 텍스처 값 또는 참조입니다.
         */
        const Asset::FMeshImportTexture& GetSpecularTexture() const;

        /**
         * @brief 자체 발광 텍스처 값을 반환합니다.
         * @return 저장한 자체 발광 텍스처 값 또는 참조입니다.
         */
        const Asset::FMeshImportTexture& GetEmissiveTexture() const;

        /**
         * @brief 환경 반사 텍스처 값을 반환합니다.
         * @return 저장한 환경 반사 텍스처 값 또는 참조입니다.
         */
        const Asset::FMeshImportTexture& GetReflectionTexture() const;

        /**
         * @brief 조명 색상·세기·정반사 지수를 검증하고 임포트한 조명 텍스처 참조와 함께 저장합니다.
         * @param[in] SpecularColor 정반사 색상 값 또는 텍스처 참조입니다.
         * @param[in] SpecularStrength 정반사 세기 값 또는 텍스처 참조입니다.
         * @param[in] Shininess 정반사 지수 값 또는 텍스처 참조입니다.
         * @param[in] AmbientColor 주변광 색상 값 또는 텍스처 참조입니다.
         * @param[in] EmissiveColor 자체 발광 색상 값 또는 텍스처 참조입니다.
         * @param[in] ReflectionColor 환경 반사 색상 값 또는 텍스처 참조입니다.
         * @param[in] ReflectionStrength 환경 반사 세기 값 또는 텍스처 참조입니다.
         * @param[in] SpecularTexture 정반사 텍스처 값 또는 텍스처 참조입니다.
         * @param[in] EmissiveTexture 자체 발광 텍스처 값 또는 텍스처 참조입니다.
         * @param[in] ReflectionTexture 환경 반사 텍스처 값 또는 텍스처 참조입니다.
         * @return 모든 조명 값이 유효해 반영되면 true입니다.
         */
        bool SetLighting(const Core::FVector4& SpecularColor, float SpecularStrength, float Shininess, const Core::FVector4& AmbientColor, const Core::FVector4& EmissiveColor, const Core::FVector4& ReflectionColor, float ReflectionStrength, Asset::FMeshImportTexture SpecularTexture, Asset::FMeshImportTexture EmissiveTexture, Asset::FMeshImportTexture ReflectionTexture);

    private:
        /**
         * @brief 정반사 색상 값 또는 참조입니다.
         */
        Core::FVector4 mSpecularColor{1.0f, 1.0f, 1.0f, 1.0f};

        /**
         * @brief 주변광 색상 값 또는 참조입니다.
         */
        Core::FVector4 mAmbientColor{1.0f, 1.0f, 1.0f, 1.0f};

        /**
         * @brief 자체 발광 색상 값 또는 참조입니다.
         */
        Core::FVector4 mEmissiveColor{0.0f, 0.0f, 0.0f, 1.0f};

        /**
         * @brief 환경 반사 색상 값 또는 참조입니다.
         */
        Core::FVector4 mReflectionColor{1.0f, 1.0f, 1.0f, 1.0f};

        /**
         * @brief 정반사 세기 값 또는 참조입니다.
         */
        float mSpecularStrength{0.0f};

        /**
         * @brief 정반사 지수 값 또는 참조입니다.
         */
        float mShininess{32.0f};

        /**
         * @brief 환경 반사 세기 값 또는 참조입니다.
         */
        float mReflectionStrength{0.0f};

        /**
         * @brief 정반사 텍스처 값 또는 참조입니다.
         */
        Asset::FMeshImportTexture mSpecularTexture{};

        /**
         * @brief 자체 발광 텍스처 값 또는 참조입니다.
         */
        Asset::FMeshImportTexture mEmissiveTexture{};

        /**
         * @brief 환경 반사 텍스처 값 또는 참조입니다.
         */
        Asset::FMeshImportTexture mReflectionTexture{};

        /**
         * @brief 저장하거나 원본 파일에서 읽은 재질 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 재질의 선형 RGBA 기본색입니다.
         */
        Core::FVector4 mBaseColor{1.0f, 1.0f, 1.0f, 1.0f};

        /**
         * @brief 뒷면을 제거하지 않고 양면으로 그릴지 나타냅니다.
         */
        bool mTwoSided{};

        /**
         * @brief 원본 재질의 기본색 텍스처 출처와 참조입니다.
         */
        Asset::FMeshImportTexture mBaseColorTexture{};

        /**
         * @brief 원본 재질의 노멀 맵 출처와 참조입니다.
         */
        Asset::FMeshImportTexture mNormalMapTexture{};
    };

}
