#include "RenderCore/Shader/FShaderDescription.h"
#include "FShaderFile.h"

#include <algorithm>
#include <string_view>

namespace {
    bool IsIdentifier(std::string_view Name) {
        if (Name.empty()) {
            return false;
        }

        for (std::size_t Index{}; Index < Name.size(); ++Index) {
            const char Character{Name[Index]};
            const bool Letter{(Character >= 'A' && Character <= 'Z') || (Character >= 'a' && Character <= 'z') || Character == '_'};
            if (!Letter && !(Index > 0 && Character >= '0' && Character <= '9')) {
                return false;
            }
        }

        return true;
    }

    bool MacroLess(const FShaderMacro& Left, const FShaderMacro& Right) {
        return Left.mName < Right.mName;
    }
}

bool FShaderDescription::Normalize(FString& Diagnostics) {
    Diagnostics.clear();
    if (!IsIdentifier(mEntryPoint) || mTargetProfile.empty() || mTargetProfile.find('\0') != FString::npos) {
        Diagnostics = "A shader requires a valid entry point and target profile";
        return false;
    }

    if (!ShaderFile::NormalizePath(mSourcePath, mSourcePath, Diagnostics)) {
        return false;
    }

    for (std::filesystem::path& Directory : mIncludeDirectories) {
        if (!ShaderFile::NormalizePath(Directory, Directory, Diagnostics)) {
            return false;
        }
    }

    std::sort(mMacros.begin(), mMacros.end(), MacroLess);
    for (std::size_t Index{}; Index < mMacros.size(); ++Index) {
        const FShaderMacro& Macro{mMacros[Index]};
        if (!IsIdentifier(Macro.mName) || Macro.mValue.find('\0') != FString::npos || (Index > 0 && mMacros[Index - 1].mName == Macro.mName)) {
            Diagnostics = "Invalid or duplicate shader macro: " + Macro.mName;
            return false;
        }
    }

    if (ShaderFile::GetCompilerIdentity().empty()) {
        Diagnostics = "Cannot identify the D3DCompile compiler binary";
        return false;
    }

    return true;
}

FString FShaderDescription::GetIdentity() const {
    TArray<Uint8> Data{};
    ShaderFile::AppendString(Data, "Macaw-D3DCompile-Shader-1");
    ShaderFile::AppendString(Data, ShaderFile::GetCompilerIdentity());
    ShaderFile::AppendString(Data, ShaderFile::ToText(mSourcePath));
    ShaderFile::AppendString(Data, mEntryPoint);
    ShaderFile::AppendString(Data, mTargetProfile);
    ShaderFile::AppendUint64(Data, mCompileFlags);
    ShaderFile::AppendUint64(Data, mMacros.size());
    for (const FShaderMacro& Macro : mMacros) {
        ShaderFile::AppendString(Data, Macro.mName);
        ShaderFile::AppendString(Data, Macro.mValue);
    }

    ShaderFile::AppendUint64(Data, mIncludeDirectories.size());
    for (const std::filesystem::path& Directory : mIncludeDirectories) {
        ShaderFile::AppendString(Data, ShaderFile::ToText(Directory));
    }

    return FString{reinterpret_cast<const char*>(Data.data()), Data.size()};
}

FString FShaderDescription::GetCacheKey() const {
    const FString Identity{GetIdentity()};
    return ShaderFile::Hash(std::span<const Uint8>{reinterpret_cast<const Uint8*>(Identity.data()), Identity.size()});
}
