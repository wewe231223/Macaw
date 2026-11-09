/**
 * @file FMaterialAssetData.h
 * @brief 재질의 저장 가능한 설정을 보관하고 직렬화·참조 검증·런타임 재질 적용을 처리합니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector4.h"

namespace Serialization {
    class FMemoryArchive;
}

namespace Asset {

    class FAssetManager;
    class UMaterial;

    /**
     * @brief 재질의 저장 가능한 설정을 보관하고 직렬화·참조 검증·런타임 재질 적용을 처리합니다.
     */
    class FMaterialAssetData {
    public:
        /**
         * @brief 기본 설정으로 FMaterialAssetData 객체를 초기화합니다.
         */
        FMaterialAssetData();

        /**
         * @brief 전달받은 설정을 보관하도록 FMaterialAssetData 객체를 초기화합니다.
         * @param[in] Material 읽거나 갱신할 재질입니다.
         */
        explicit FMaterialAssetData(const Asset::UMaterial& Material);

    public:
        /**
         * @brief 재질 설정을 바이너리 아카이브에 기록하거나 읽고 지원하지 않는 버전을 거부합니다.
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         */
        void Serialize(Serialization::FMemoryArchive& Archive);

        /**
         * @brief 저장한 재질 설정을 각 Setter의 검증을 거쳐 런타임 재질에 적용합니다.
         * @param[in,out] Material 읽거나 갱신할 재질입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 모든 값을 수락하면 true이며 하나라도 유효하지 않으면 Error를 기록하고 false입니다.
         */
        bool ApplyTo(Asset::UMaterial& Material, Core::FString& Error) const;

        /**
         * @brief 재질 이름과 비어 있지 않은 셰이더·텍스처 경로가 프로젝트 경계 안에 있는지 검증합니다.
         * @param[in] Assets 경로 경계를 확인하고 에셋을 가져올 프로젝트 에셋 관리자입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 이름과 모든 참조 경로가 유효하면 true이며 그렇지 않으면 false입니다.
         */
        bool ValidateReferences(const Asset::FAssetManager& Assets, Core::FString& Error) const;

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
         * @brief 환경 반사 세기 값 또는 참조입니다.
         */
        float mReflectionStrength{};

        /**
         * @brief 정반사 텍스처의 프로젝트 상대 경로 값 또는 참조입니다.
         */
        Core::FString mSpecularTexturePath{};

        /**
         * @brief 자체 발광 텍스처의 프로젝트 상대 경로 값 또는 참조입니다.
         */
        Core::FString mEmissiveTexturePath{};

        /**
         * @brief 환경 반사 텍스처의 프로젝트 상대 경로 값 또는 참조입니다.
         */
        Core::FString mReflectionTexturePath{};

        /**
         * @brief 저장하거나 원본 파일에서 읽은 재질 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief Content 폴더 기준 재질 셰이더의 상대 경로입니다.
         */
        Core::FString mShaderPath{};

        /**
         * @brief 재질의 선형 RGBA 기본색입니다.
         */
        Core::FVector4 mBaseColor{};

        /**
         * @brief 기본색 텍스처의 프로젝트 상대 경로이며 빈 값은 텍스처 없음을 나타냅니다.
         */
        Core::FString mBaseColorTexturePath{};

        /**
         * @brief 노멀 맵의 프로젝트 상대 경로이며 빈 값은 노멀 맵 없음을 나타냅니다.
         */
        Core::FString mNormalMapTexturePath{};

        /**
         * @brief 재질의 정반사 기여도 배율입니다.
         */
        float mSpecularStrength{};

        /**
         * @brief Phong 정반사 하이라이트의 지수입니다.
         */
        float mShininess{};

        /**
         * @brief 접선 공간 노멀 맵의 강도 배율입니다.
         */
        float mNormalStrength{};

        /**
         * @brief 뒷면을 제거하지 않고 양면으로 그릴지 나타냅니다.
         */
        bool mTwoSided{};
    };

}
