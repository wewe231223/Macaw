#include "RenderCore/Shader/FShader.h"

#include <utility>

FShader::FShader(FShaderDescription Description, FString CacheKey, FShaderCompilation Compilation, bool LoadedFromCache)
	: mDescription{std::move(Description)},
	  mCacheKey{std::move(CacheKey)},
	  mCompilation{std::make_shared<const FShaderCompilation>(std::move(Compilation))},
	  mLoadedFromCache{LoadedFromCache} {
}

const FShaderDescription& FShader::GetDescription() const {
    return mDescription;
}

const FString& FShader::GetCacheKey() const {
    return mCacheKey;
}

std::span<const Uint8> FShader::GetByteCode() const {
    return mCompilation->mByteCode;
}

std::shared_ptr<const FShaderCompilation> FShader::GetCompilation() const {
    return mCompilation;
}

Uint64 FShader::GetRevision() const {
    return mRevision;
}

bool FShader::IsLoadedFromCache() const {
    return mLoadedFromCache;
}

void FShader::Commit(FShaderCompilation Compilation, bool LoadedFromCache) {
    mCompilation = std::make_shared<const FShaderCompilation>(std::move(Compilation));
    mLoadedFromCache = LoadedFromCache;
    ++mRevision;
}
