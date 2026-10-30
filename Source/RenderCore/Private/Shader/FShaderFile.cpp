/**
 * @file FShaderFile.cpp
 * @brief 셰이더 경로·파일 의존성·해시와 캐시 데이터 입출력에 대한 구현입니다.
 * @ingroup RenderCore
 */

#include "FShaderFile.h"
#include "Core/FilePath.h"

#include <Windows.h>
#include <bcrypt.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <fstream>
#include <limits>

namespace RenderCore {

    /**
     * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
     */
    namespace {
        /**
         * @brief 셰이더 파일을 읽을 때 허용하는 최대 바이트 수입니다.
         */
        constexpr Core::Uint64 MaxFileSize{64 * 1024 * 1024};

        /**
         * @brief 사용하는 컴파일러 모듈의 정보를 읽어 캐시 식별 문자열을 만듭니다.
         * @return 컴파일러를 구분하는 문자열이며 식별 실패 시 비어 있습니다.
         */
        Core::FString MakeCompilerIdentity() {
            const HMODULE Module{GetModuleHandleW(D3DCOMPILER_DLL_W)};

            if (Module == nullptr) {
                return {};
            }

            std::array<wchar_t, 32768> Path{};
            const DWORD Length{GetModuleFileNameW(Module, Path.data(), static_cast<DWORD>(Path.size()))};

            if (Length == 0 || Length >= Path.size()) {
                return {};
            }

            Core::TArray<Core::Uint8> Data{};
            RenderCore::FShaderDependency Dependency{};
            Core::FString Diagnostics{};

            if (!RenderCore::ShaderFile::Read(std::filesystem::path{Path.data()}, Data, Dependency, Diagnostics)) {
                return {};
            }

            return RenderCore::ShaderFile::Hash(Data);
        }
    }

    bool RenderCore::ShaderFile::NormalizePath(const std::filesystem::path& Path, std::filesystem::path& Normalized, Core::FString& Diagnostics) {
        if (Path.empty()) {
            Diagnostics = "Shader paths must not be empty";
            return false;
        }

        std::error_code Error{};
        Normalized = std::filesystem::absolute(Path, Error).lexically_normal();

        if (Error) {
            Diagnostics = "Invalid shader path: " + Core::FilePath::ToGenericUtf8(Path);
            return false;
        }

        return true;
    }

    bool RenderCore::ShaderFile::Inspect(const std::filesystem::path& Path, RenderCore::FShaderDependency& Dependency) {
        Dependency = {};
        Dependency.mPath = Path;

        std::error_code Error{};
        Dependency.mExists = std::filesystem::is_regular_file(Path, Error);

        if (Error) {
            return Error == std::errc::no_such_file_or_directory || Error == std::errc::not_a_directory;
        }

        if (!Dependency.mExists) {
            return true;
        }

        Dependency.mWriteTime = std::filesystem::last_write_time(Path, Error);

        if (Error) {
            return false;
        }

        Dependency.mSize = std::filesystem::file_size(Path, Error);

        return !Error;
    }

    bool RenderCore::ShaderFile::Read(const std::filesystem::path& Path, Core::TArray<Core::Uint8>& Data, RenderCore::FShaderDependency& Dependency, Core::FString& Diagnostics) {
        if (!RenderCore::ShaderFile::Inspect(Path, Dependency) || !Dependency.mExists || Dependency.mSize > MaxFileSize) {
            Diagnostics = "Cannot read shader file: " + Core::FilePath::ToGenericUtf8(Path);
            return false;
        }

        std::ifstream Stream{Path, std::ios::binary};

        Data.resize(static_cast<std::size_t>(Dependency.mSize));

        if (!Stream || (!Data.empty() && !Stream.read(reinterpret_cast<char*>(Data.data()), static_cast<std::streamsize>(Data.size())))) {
            Diagnostics = "Cannot read shader file: " + Core::FilePath::ToGenericUtf8(Path);
            return false;
        }

        RenderCore::FShaderDependency Current{};

        if (!RenderCore::ShaderFile::Inspect(Path, Current) || !Current.mExists || Current.mWriteTime != Dependency.mWriteTime || Current.mSize != Dependency.mSize) {
            Diagnostics = "Shader file changed while reading: " + Core::FilePath::ToGenericUtf8(Path);
            return false;
        }

        return true;
    }

    bool RenderCore::ShaderFile::IsSnapshotCurrent(const Core::TArray<RenderCore::FShaderDependency>& Dependencies) {
        for (const RenderCore::FShaderDependency& Dependency : Dependencies) {
            RenderCore::FShaderDependency Current{};

            if (!RenderCore::ShaderFile::Inspect(Dependency.mPath, Current) || Current.mExists != Dependency.mExists || (Current.mExists && (Current.mWriteTime != Dependency.mWriteTime || Current.mSize != Dependency.mSize))) {
                return false;
            }
        }

        return true;
    }

    Core::FString RenderCore::ShaderFile::Hash(std::span<const Core::Uint8> Data) {
        if (Data.size() > std::numeric_limits<ULONG>::max()) {
            return {};
        }

        BCRYPT_ALG_HANDLE Algorithm{};

        if (BCryptOpenAlgorithmProvider(&Algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) {
            return {};
        }

        std::array<Core::Uint8, 32> Digest{};
        const NTSTATUS Status{BCryptHash(Algorithm, nullptr, 0, const_cast<PUCHAR>(Data.data()), static_cast<ULONG>(Data.size()), Digest.data(), static_cast<ULONG>(Digest.size()))};

        BCryptCloseAlgorithmProvider(Algorithm, 0);

        if (Status < 0) {
            return {};
        }

        constexpr char Digits[]{"0123456789abcdef"};
        Core::FString Result{};
        Result.reserve(Digest.size() * 2);

        for (const Core::Uint8 Byte : Digest) {
            Result.push_back(Digits[Byte >> 4]);
            Result.push_back(Digits[Byte & 15]);
        }

        return Result;
    }

    const Core::FString& RenderCore::ShaderFile::GetCompilerIdentity() {
        static const Core::FString Identity{MakeCompilerIdentity()};
        return Identity;
    }

    void RenderCore::ShaderFile::AppendUint64(Core::TArray<Core::Uint8>& Data, Core::Uint64 Value) {
        for (Core::Uint32 Shift{}; Shift < 64; Shift += 8) {
            Data.push_back(static_cast<Core::Uint8>(Value >> Shift));
        }
    }

    bool RenderCore::ShaderFile::ReadUint64(std::span<const Core::Uint8> Data, std::size_t& Position, Core::Uint64& Value) {
        if (Position > Data.size() || Data.size() - Position < sizeof(Core::Uint64)) {
            return false;
        }

        Value = 0;

        for (Core::Uint32 Shift{}; Shift < 64; Shift += 8) {
            Value |= static_cast<Core::Uint64>(Data[Position++]) << Shift;
        }

        return true;
    }

    void RenderCore::ShaderFile::AppendString(Core::TArray<Core::Uint8>& Data, std::string_view Value) {
        RenderCore::ShaderFile::AppendUint64(Data, Value.size());
        Data.insert(Data.end(), Value.begin(), Value.end());
    }

    bool RenderCore::ShaderFile::ReadString(std::span<const Core::Uint8> Data, std::size_t& Position, Core::FString& Value) {
        Core::Uint64 Size{};

        if (!RenderCore::ShaderFile::ReadUint64(Data, Position, Size) || Size > 1024 * 1024 || Size > Data.size() - Position) {
            return false;
        }

        Value.assign(reinterpret_cast<const char*>(Data.data() + Position), static_cast<std::size_t>(Size));
        Position += static_cast<std::size_t>(Size);

        return true;
    }

}
