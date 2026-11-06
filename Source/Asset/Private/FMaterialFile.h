/**
 * @file FMaterialFile.h
 * @brief 재질 에셋의 검증, 로드와 안전한 저장을 선언합니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <filesystem>

namespace Asset {

    class FAssetManager;
    class UMaterial;

    /**
     * @brief 프로젝트 상대 참조를 검증하며 재질 에셋을 읽고 기록합니다.
     */
    class FMaterialFile {
    public:
        /**
         * @brief 단일 머티리얼 설정과 텍스처 참조를 읽고 검증합니다.
         * @param[in] Path 재질 에셋 파일 경로입니다.
         * @param[in,out] Assets 참조를 검증하고 텍스처를 로드할 관리자입니다.
         * @param[in,out] Material 읽은 설정을 적용할 대상입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 모든 설정과 텍스처를 읽으면 true입니다.
         */
        static bool Load(const std::filesystem::path& Path, Asset::FAssetManager& Assets, Asset::UMaterial& Material, Core::FString& Error);

        /**
         * @brief 설정을 기록한 임시 파일로 기존 재질 에셋 파일을 교체합니다.
         * @param[in] Path 저장할 재질 에셋 파일 경로입니다.
         * @param[in] Assets 프로젝트 경로를 검증할 관리자입니다.
         * @param[in] Material 저장할 설정입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 검증과 파일 교체에 성공하면 true입니다.
         */
        static bool Save(const std::filesystem::path& Path, const Asset::FAssetManager& Assets, const Asset::UMaterial& Material, Core::FString& Error);
    };

}
