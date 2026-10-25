/**
 * @file FShaderManager.cpp
 * @brief 셰이더 캐시 로드와 변경 감지 및 재컴파일에 대한 구현입니다.
 * @ingroup RenderCore
 */

#include "RenderCore/Shader/FShaderManager.h"

#include <utility>

FShaderManager::FShaderManager(std::filesystem::path CacheDirectory)
	: mCache{std::move(CacheDirectory)} {
}

std::shared_ptr<const FShader> FShaderManager::Load(const FShaderDescription& Description, FString& Diagnostics) {
    FShaderDescription Normalized{Description};
    if (!Normalized.Normalize(Diagnostics)) {
        return nullptr;
    }

    const FString Key{Normalized.GetCacheKey()};
    if (Key.empty()) {
        Diagnostics = "Cannot compute shader cache key";
        return nullptr;
    }

    const auto Existing{mShaders.find(Key)};
    if (Existing != mShaders.end()) {
        return Existing->second;
    }

    FShaderCompilation Compilation{};
    bool LoadedFromCache{};
    if (!LoadCompilation(Normalized, Compilation, LoadedFromCache, Diagnostics)) {
        return nullptr;
    }

    auto Shader{std::make_shared<FShader>(std::move(Normalized), Key, std::move(Compilation), LoadedFromCache)};
    mShaders.emplace(Key, Shader);
    return Shader;
}

bool FShaderManager::HasChanges(const FShader& Shader) const {
    return !mCache.IsCurrent(*Shader.GetCompilation());
}

bool FShaderManager::ReloadChanged(TArray<std::shared_ptr<const FShader>>& ChangedShaders, FString& Diagnostics) {
    ChangedShaders.clear();
    Diagnostics.clear();
    bool Success{true};
    for (const auto& Entry : mShaders) {
        const std::shared_ptr<FShader>& Shader{Entry.second};
        if (!HasChanges(*Shader)) {
            continue;
        }

        FShaderCompilation Compilation{};
        bool LoadedFromCache{};
        FString Message{};
        if (!LoadCompilation(Shader->GetDescription(), Compilation, LoadedFromCache, Message)) {
            Success = false;
        } else {
            Shader->Commit(std::move(Compilation), LoadedFromCache);
            ChangedShaders.push_back(Shader);
        }

        if (!Message.empty()) {
            Diagnostics += Message + "\n";
        }
    }

    return Success;
}

void FShaderManager::Clear() {
    mShaders.clear();
}

bool FShaderManager::LoadCompilation(const FShaderDescription& Description, FShaderCompilation& Compilation, bool& LoadedFromCache, FString& Diagnostics) const {
    LoadedFromCache = mCache.Load(Description, Compilation, Diagnostics);
    if (LoadedFromCache) {
        return true;
    }

    if (!mCompiler.Compile(Description, Compilation, Diagnostics)) {
        return false;
    }

    FString CacheDiagnostics{};
    if (!mCache.Save(Description, Compilation, CacheDiagnostics)) {
        Diagnostics += "\n" + CacheDiagnostics;
    }

    return true;
}
