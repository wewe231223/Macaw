/**
 * @file FShaderCompiler.cpp
 * @brief HLSL 컴파일과 진단 및 include 의존성 수집에 대한 구현입니다.
 * @ingroup RenderCore
 */

#include "RenderCore/Shader/FShaderCompiler.h"
#include "Core/FilePath.h"
#include "FShaderIncludeHandler.h"
#include "FShaderFile.h"

#include <wrl/client.h>

#include <utility>

using namespace Microsoft::WRL;

bool FShaderCompiler::Compile(const FShaderDescription& Description, FShaderCompilation& Compilation, FString& Diagnostics) const {
    FShaderDescription Normalized{Description};
    if (!Normalized.Normalize(Diagnostics)) {
        return false;
    }

    TArray<Uint8> Source{};
    FShaderDependency RootDependency{};
    if (!ShaderFile::Read(Normalized.mSourcePath, Source, RootDependency, Diagnostics)) {
        return false;
    }

    if (Source.empty()) {
        Diagnostics = "Shader source is empty: " + FilePath::ToGenericUtf8(Normalized.mSourcePath);
        return false;
    }

    TArray<D3D_SHADER_MACRO> Macros{};
    Macros.reserve(Normalized.mMacros.size() + 1);
    for (const FShaderMacro& Macro : Normalized.mMacros) {
        Macros.push_back({Macro.mName.c_str(), Macro.mValue.c_str()});
    }

    Macros.push_back({nullptr, nullptr});
    FShaderIncludeHandler Includes{Normalized, Source.data(), RootDependency};
    ComPtr<ID3DBlob> Code{};
    ComPtr<ID3DBlob> Errors{};
    const FString SourceName{FilePath::ToGenericUtf8(Normalized.mSourcePath)};
    const HRESULT Result{D3DCompile(Source.data(), Source.size(), SourceName.c_str(), Macros.data(), &Includes, Normalized.mEntryPoint.c_str(), Normalized.mTargetProfile.c_str(), Normalized.mCompileFlags, 0, Code.GetAddressOf(), Errors.GetAddressOf())};
    if (Errors != nullptr) {
        Diagnostics.assign(static_cast<const char*>(Errors->GetBufferPointer()), Errors->GetBufferSize());
        while (!Diagnostics.empty() && Diagnostics.back() == '\0') {
            Diagnostics.pop_back();
        }
    }

    if (FAILED(Result) || Code == nullptr) {
        if (!Includes.GetDiagnostics().empty()) {
            Diagnostics += "\n" + Includes.GetDiagnostics();
        }

        if (Diagnostics.empty()) {
            Diagnostics = "D3DCompile failed: " + SourceName;
        }

        return false;
    }

    const auto BuildTime{std::filesystem::file_time_type::clock::now()};
    if (!ShaderFile::IsSnapshotCurrent(Includes.GetDependencies())) {
        Diagnostics = "Shader dependencies changed during compilation; retry the request: " + SourceName;
        return false;
    }

    FShaderCompilation Candidate{};
    const auto* Begin{static_cast<const Uint8*>(Code->GetBufferPointer())};
    Candidate.mByteCode.assign(Begin, Begin + Code->GetBufferSize());
    Candidate.mDependencies = Includes.GetDependencies();
    Candidate.mBuildTime = BuildTime;
    Compilation = std::move(Candidate);
    return true;
}
