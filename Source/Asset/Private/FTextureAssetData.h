/**
 * @file FTextureAssetData.h
 * @brief 텍스처 에셋의 공통 저장 필드와 복원 절차를 선언합니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

namespace Serialization {
    class FMemoryArchive;
}

namespace Asset {

    class UTexture2D;

    /**
     * @brief 기존 바이너리 필드 순서를 유지하며 텍스처 에셋 데이터를 저장하고 복원합니다.
     */
    class FTextureAssetData {
    public:
        /**
         * @brief 로드할 필드를 기본값으로 초기화합니다.
         */
        FTextureAssetData() = default;

        /**
         * @brief 텍스처의 저장 가능한 이미지와 메타데이터를 복사합니다.
         * @param[in] Texture 원본 텍스처입니다.
         */
        explicit FTextureAssetData(const Asset::UTexture2D& Texture);

    public:
        /**
         * @brief 크기, 포맷, 색 공간, DDS 여부와 이미지 바이트를 같은 순서로 직렬화합니다.
         * @param[in,out] Archive 저장 또는 로드할 메모리 아카이브입니다.
         */
        void Serialize(Serialization::FMemoryArchive& Archive);

        /**
         * @brief 포맷과 이미지 메타데이터를 검증하여 텍스처에 적용합니다.
         * @param[in,out] Texture 데이터를 적용할 텍스처입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 이미지 복원과 메타데이터 검증에 성공하면 true입니다.
         */
        bool ApplyTo(Asset::UTexture2D& Texture, Core::FString& Error) const;

    private:
        /**
         * @brief 최상위 밉의 가로 픽셀 수입니다.
         */
        Core::Uint32 mWidth{};

        /**
         * @brief 최상위 밉의 세로 픽셀 수입니다.
         */
        Core::Uint32 mHeight{};

        /**
         * @brief 바이너리에 기록하는 텍스처 포맷 값입니다.
         */
        Core::Uint32 mFormat{};

        /**
         * @brief 이미지를 sRGB 색 공간으로 해석하는지 나타냅니다.
         */
        bool mSRGB{};

        /**
         * @brief 이미지 바이트가 DDS 컨테이너인지 나타냅니다.
         */
        bool mDDS{};

        /**
         * @brief 길이가 보존되는 바이너리 이미지 바이트입니다.
         */
        Core::FString mImageData{};
    };

}
