#pragma once

#include "RenderCore/Shader/FShader.h"
#include "RenderCore/Shader/FShaderCache.h"
#include "RenderCore/Shader/FShaderCompiler.h"

class FShaderManager {
public:
    ReflectTypes(FShaderManager);

public:
    explicit FShaderManager(std::filesystem::path CacheDirectory = "Intermediate/ShaderCache");
    ~FShaderManager() = default;
    FShaderManager(const FShaderManager&) = delete;
    FShaderManager& operator=(const FShaderManager&) = delete;
    FShaderManager(FShaderManager&&) = delete;
    FShaderManager& operator=(FShaderManager&&) = delete;

public:
    ReflectStruct(FShaderManager);
    std::shared_ptr<const FShader> Load(const FShaderDescription& Description, FString& Diagnostics);
    bool HasChanges(const FShader& Shader) const;
    bool ReloadChanged(TArray<std::shared_ptr<const FShader>>& ChangedShaders, FString& Diagnostics);
    void Clear();

private:
    ReflectMembers();
    bool LoadCompilation(const FShaderDescription& Description, FShaderCompilation& Compilation, bool& LoadedFromCache, FString& Diagnostics) const;

private:
    FShaderCompiler mCompiler{};
    ReflectProperty(mCompiler, "Compiler", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FShaderCache mCache{};
    ReflectProperty(mCache, "Cache", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TMap<FString, std::shared_ptr<FShader>> mShaders{};
    ReflectProperty(mShaders, "Shaders", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
