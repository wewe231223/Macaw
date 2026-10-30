/**
 * @file FAssetImportScope.cpp
 * @brief 임포트 전용 디렉터리의 롤백 구현입니다.
 * @ingroup Asset
 */
#include "FAssetImportScope.h"

#include <utility>

namespace Asset {

    Asset::FAssetImportScope::FAssetImportScope(std::filesystem::path Directory)
		: mDirectory{std::move(Directory)} {
    }

    Asset::FAssetImportScope::~FAssetImportScope() {
        if (!mCommitted) {
            std::error_code Code{};
            std::filesystem::remove_all(mDirectory, Code);
        }
    }

    void Asset::FAssetImportScope::Commit() {
        mCommitted = true;
    }

}
