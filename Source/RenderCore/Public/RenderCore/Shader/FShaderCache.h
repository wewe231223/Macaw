#pragma once

#include "RenderCore/Shader/FShaderCompilation.h"
#include "RenderCore/Shader/FShaderDescription.h"

class FShaderCache {
public:
    ReflectTypes(FShaderCache);

public:
    explicit FShaderCache(std::filesystem::path Directory = "Intermediate/ShaderCache");

public:
    ReflectStruct(FShaderCache);
    std::filesystem::path GetPath(const FString& CacheKey) const;
    bool Load(const FShaderDescription& Description, FShaderCompilation& Compilation, FString& Diagnostics) const;
    bool Save(const FShaderDescription& Description, const FShaderCompilation& Compilation, FString& Diagnostics) const;
    bool IsCurrent(const FShaderCompilation& Compilation) const;

private:
    ReflectMembers();

private:
    std::filesystem::path mDirectory{};
    ReflectProperty(mDirectory, "Directory", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
