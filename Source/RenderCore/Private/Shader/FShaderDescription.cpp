/**
 * @file FShaderDescription.cpp
 * @brief 셰이더 경로·진입점·매크로 정규화와 캐시 키 생성에 대한 구현입니다.
 * @ingroup RenderCore
 */

#include "RenderCore/Shader/FShaderDescription.h"
#include "Core/FilePath.h"
#include "FShaderFile.h"

#include <algorithm>
#include <string_view>

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 문자열이 셰이더 매크로 이름으로 사용할 수 있는 식별자인지 검사합니다.
     * @param[in] Name 검사할 셰이더 매크로 또는 진입점의 식별자입니다.
     * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
     */
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

    /**
     * @brief 셰이더 매크로를 이름에 따라 결정적인 순서로 비교합니다.
     * @param[in] Left 비교하거나 결합할 왼쪽 값입니다.
     * @param[in] Right 비교하거나 결합할 오른쪽 값입니다.
     * @return 왼쪽 매크로의 이름이 정렬 순서에서 앞서면 true입니다.
     */
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
    ShaderFile::AppendString(Data, FilePath::ToGenericUtf8(mSourcePath));
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
        ShaderFile::AppendString(Data, FilePath::ToGenericUtf8(Directory));
    }

    return FString{reinterpret_cast<const char*>(Data.data()), Data.size()};
}

FString FShaderDescription::GetCacheKey() const {
    const FString Identity{GetIdentity()};
    return ShaderFile::Hash(std::span<const Uint8>{reinterpret_cast<const Uint8*>(Identity.data()), Identity.size()});
}
