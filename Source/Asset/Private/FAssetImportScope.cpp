/**
 * @file FAssetImportScope.cpp
 * @brief 임포트 전용 디렉터리의 롤백 구현입니다.
 * @ingroup Asset
 */

#include "FAssetImportScope.h"

#include <utility>

namespace Asset {

    /**
     * @brief 소유할 새 디렉터리 경로를 저장합니다.
     */
    Asset::FAssetImportScope::FAssetImportScope(std::filesystem::path Directory)
		: mDirectory{std::move(Directory)} {
    }

    /**
     * @brief 성공 확정 전이라면 이번 임포트가 만든 파일을 제거합니다.
     */
    Asset::FAssetImportScope::~FAssetImportScope() {
        if (!mCommitted) {
            std::error_code Code{};

            std::filesystem::remove_all(mDirectory, Code);
        }
    }

    /**
     * @brief 임포트가 완성되어 의존 에셋을 유지하도록 표시합니다.
     */
    void Asset::FAssetImportScope::Commit() {
        mCommitted = true;
    }

}
