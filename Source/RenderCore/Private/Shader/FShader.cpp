/**
 * @file FShader.cpp
 * @brief 셰이더 설명과 공유 컴파일 결과 및 갱신 세대 관리에 대한 구현입니다.
 * @ingroup RenderCore
 */

#include "RenderCore/Shader/FShader.h"

#include <utility>

namespace RenderCore {

    RenderCore::FShader::FShader(RenderCore::FShaderDescription Description, Core::FString CacheKey, RenderCore::FShaderCompilation Compilation, bool LoadedFromCache)
		: mDescription{std::move(Description)},
		  mCacheKey{std::move(CacheKey)},
		  mCompilation{std::make_shared<const RenderCore::FShaderCompilation>(std::move(Compilation))},
		  mLoadedFromCache{LoadedFromCache} {
    }

    const RenderCore::FShaderDescription& RenderCore::FShader::GetDescription() const {
        return mDescription;
    }

    const Core::FString& RenderCore::FShader::GetCacheKey() const {
        return mCacheKey;
    }

    std::span<const Core::Uint8> RenderCore::FShader::GetByteCode() const {
        return mCompilation->mByteCode;
    }

    std::shared_ptr<const RenderCore::FShaderCompilation> RenderCore::FShader::GetCompilation() const {
        return mCompilation;
    }

    Core::Uint64 RenderCore::FShader::GetRevision() const {
        return mRevision;
    }

    bool RenderCore::FShader::IsLoadedFromCache() const {
        return mLoadedFromCache;
    }

    void RenderCore::FShader::Commit(RenderCore::FShaderCompilation Compilation, bool LoadedFromCache) {
        mCompilation = std::make_shared<const RenderCore::FShaderCompilation>(std::move(Compilation));
        mLoadedFromCache = LoadedFromCache;
        ++mRevision;
    }

}
