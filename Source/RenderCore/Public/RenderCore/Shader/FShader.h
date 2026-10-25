#pragma once

#include "RenderCore/Shader/FShaderCompilation.h"
#include "RenderCore/Shader/FShaderDescription.h"

#include <span>

class FShaderManager;

class FShader {
public:
    ReflectTypes(FShader);

public:
    FShader(FShaderDescription Description, FString CacheKey, FShaderCompilation Compilation, bool LoadedFromCache);
    ~FShader() = default;
    FShader(const FShader&) = delete;
    FShader& operator=(const FShader&) = delete;
    FShader(FShader&&) = delete;
    FShader& operator=(FShader&&) = delete;

public:
    ReflectStruct(FShader);
    const FShaderDescription& GetDescription() const;
    const FString& GetCacheKey() const;
    std::span<const Uint8> GetByteCode() const;
    std::shared_ptr<const FShaderCompilation> GetCompilation() const;
    Uint64 GetRevision() const;
    bool IsLoadedFromCache() const;

private:
    ReflectMembers();
    friend class FShaderManager;

    void Commit(FShaderCompilation Compilation, bool LoadedFromCache);

private:
    FShaderDescription mDescription{};
    ReflectProperty(mDescription, "Description", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FString mCacheKey{};
    ReflectProperty(mCacheKey, "CacheKey", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::shared_ptr<const FShaderCompilation> mCompilation{};
    ReflectProperty(mCompilation, "Compilation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mRevision{1};
    ReflectProperty(mRevision, "Revision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mLoadedFromCache{};
    ReflectProperty(mLoadedFromCache, "LoadedFromCache", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
