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

namespace RenderCore {

    bool RenderCore::FShaderCompiler::Compile(const RenderCore::FShaderDescription& Description, RenderCore::FShaderCompilation& Compilation, Core::FString& Diagnostics) const {
        RenderCore::FShaderDescription Normalized{Description};

        if (!Normalized.Normalize(Diagnostics)) {
            return false;
        }

        Core::TArray<Core::Uint8> Source{};
        RenderCore::FShaderDependency RootDependency{};

        if (!RenderCore::ShaderFile::Read(Normalized.mSourcePath, Source, RootDependency, Diagnostics)) {
            return false;
        }

        if (Source.empty()) {
            Diagnostics = "Shader source is empty: " + Core::FilePath::ToGenericUtf8(Normalized.mSourcePath);
            return false;
        }

        Core::TArray<D3D_SHADER_MACRO> Macros{};
        Macros.reserve(Normalized.mMacros.size() + 1);

        for (const RenderCore::FShaderMacro& Macro : Normalized.mMacros) {
            Macros.push_back({Macro.mName.c_str(), Macro.mValue.c_str()});
        }

        Macros.push_back({nullptr, nullptr});

        RenderCore::FShaderIncludeHandler Includes{Normalized, Source.data(), RootDependency};
        Microsoft::WRL::ComPtr<ID3DBlob> Code{};
        Microsoft::WRL::ComPtr<ID3DBlob> Errors{};
        const Core::FString SourceName{Core::FilePath::ToGenericUtf8(Normalized.mSourcePath)};
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

        if (!RenderCore::ShaderFile::IsSnapshotCurrent(Includes.GetDependencies())) {
            Diagnostics = "Shader dependencies changed during compilation; retry the request: " + SourceName;
            return false;
        }

        RenderCore::FShaderCompilation Candidate{};
        const auto* Begin{static_cast<const Core::Uint8*>(Code->GetBufferPointer())};
        Candidate.mByteCode.assign(Begin, Begin + Code->GetBufferSize());
        Candidate.mDependencies = Includes.GetDependencies();
        Candidate.mBuildTime = BuildTime;
        Compilation = std::move(Candidate);

        return true;
    }

}
