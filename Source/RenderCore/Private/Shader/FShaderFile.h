#pragma once

#include "RenderCore/Shader/FShaderCompilation.h"

#include <span>
#include <string_view>

namespace ShaderFile {
    FString ToText(const std::filesystem::path& Path);
    std::filesystem::path FromText(std::string_view Text);
    bool NormalizePath(const std::filesystem::path& Path, std::filesystem::path& Normalized, FString& Diagnostics);
    bool Inspect(const std::filesystem::path& Path, FShaderDependency& Dependency);
    bool Read(const std::filesystem::path& Path, TArray<Uint8>& Data, FShaderDependency& Dependency, FString& Diagnostics);
    bool IsSnapshotCurrent(const TArray<FShaderDependency>& Dependencies);
    FString Hash(std::span<const Uint8> Data);
    const FString& GetCompilerIdentity();
    void AppendUint64(TArray<Uint8>& Data, Uint64 Value);
    bool ReadUint64(std::span<const Uint8> Data, std::size_t& Position, Uint64& Value);
    void AppendString(TArray<Uint8>& Data, std::string_view Value);
    bool ReadString(std::span<const Uint8> Data, std::size_t& Position, FString& Value);
}
