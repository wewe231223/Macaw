/**
 * @file FAssetImportScope.h
 * @brief 임포트 실패 시 새로 만든 의존 에셋을 정리합니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <filesystem>
#include <vector>

namespace Asset {

    /**
     * @brief 한 번의 임포트에 전용인 새 디렉터리를 성공 확정 전까지 관리합니다.
     */
    class FAssetImportScope {
    public:

        /**
         * @brief 완료를 확정하지 않으면 생성 파일을 롤백하는 임포트 범위를 시작합니다.
         */
        FAssetImportScope() = default;

        /**
         * @brief 소유할 새 디렉터리 경로를 저장합니다.
         * @param Directory 이번 작업이 새로 만든 전용 경로입니다.
         */
        explicit FAssetImportScope(std::filesystem::path Directory);

        /**
         * @brief 성공 확정 전이라면 이번 임포트가 만든 파일을 제거합니다.
         */
        ~FAssetImportScope();

        /**
         * @brief 롤백 소유권의 복사를 금지합니다.
         * @param Other 복사 대상입니다.
         */
        FAssetImportScope(const Asset::FAssetImportScope& Other) = delete;

        /**
         * @brief 롤백 소유권의 복사를 금지합니다.
         * @param Other 대입 대상입니다.
         * @return 이 객체의 참조입니다.
         */
        Asset::FAssetImportScope& operator=(const Asset::FAssetImportScope& Other) = delete;

    public:

        /**
         * @brief 임포트 실패 시 삭제할 생성 파일을 롤백 목록에 등록합니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         */
        void TrackFile(std::filesystem::path Path);

        /**
         * @brief 임포트가 완성되어 의존 에셋을 유지하도록 표시합니다.
         */
        void Commit();

    private:

        /**
         * @brief 이번 임포트만 사용하는 의존 에셋 디렉터리입니다.
         */
        std::filesystem::path mDirectory{};

        /**
         * @brief 임포트 실패 시 롤백할 생성 파일 목록입니다.
         */
        std::vector<std::filesystem::path> mFiles{};

        /**
         * @brief 메시 에셋 저장까지 성공했는지 여부입니다.
         */
        bool mCommitted{};
    };

}
