/**
 * @file FAssetImportScope.cpp
 * @brief 임포트 전용 디렉터리의 롤백 구현입니다.
 * @ingroup Engine
 */
#include "FAssetImportScope.h"

#include <utility>

namespace Engine {

    Engine::FAssetImportScope::FAssetImportScope(std::filesystem::path Directory)
		: mDirectory{std::move(Directory)} {
    }

    Engine::FAssetImportScope::~FAssetImportScope() {
        if (!mCommitted) {
            std::error_code Code{};
            std::filesystem::remove_all(mDirectory, Code);
        }
    }

    void Engine::FAssetImportScope::Commit() {
        mCommitted = true;
    }

}
