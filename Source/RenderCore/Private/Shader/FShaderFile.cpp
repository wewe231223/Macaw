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

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 셰이더 파일을 읽을 때 허용하는 최대 바이트 수입니다.
     */
    constexpr Uint64 MaxFileSize{64 * 1024 * 1024};

    /**
     * @brief 사용하는 컴파일러 모듈의 정보를 읽어 캐시 식별 문자열을 만듭니다.
     * @return 컴파일러를 구분하는 문자열이며 식별 실패 시 비어 있습니다.
     */
    FString MakeCompilerIdentity() {
        const HMODULE Module{GetModuleHandleW(D3DCOMPILER_DLL_W)};

        if (Module == nullptr) {
            return {};
        }

        std::array<wchar_t, 32768> Path{};
        const DWORD Length{GetModuleFileNameW(Module, Path.data(), static_cast<DWORD>(Path.size()))};

        if (Length == 0 || Length >= Path.size()) {
            return {};
        }

        TArray<Uint8> Data{};
        FShaderDependency Dependency{};
        FString Diagnostics{};

        if (!ShaderFile::Read(std::filesystem::path{Path.data()}, Data, Dependency, Diagnostics)) {
            return {};
        }

        return ShaderFile::Hash(Data);
    }
}

bool ShaderFile::NormalizePath(const std::filesystem::path& Path, std::filesystem::path& Normalized, FString& Diagnostics) {
    if (Path.empty()) {
        Diagnostics = "Shader paths must not be empty";
        return false;
    }

    std::error_code Error{};
    Normalized = std::filesystem::absolute(Path, Error).lexically_normal();

    if (Error) {
        Diagnostics = "Invalid shader path: " + FilePath::ToGenericUtf8(Path);
        return false;
    }

    return true;
}

bool ShaderFile::Inspect(const std::filesystem::path& Path, FShaderDependency& Dependency) {
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

bool ShaderFile::Read(const std::filesystem::path& Path, TArray<Uint8>& Data, FShaderDependency& Dependency, FString& Diagnostics) {
    if (!Inspect(Path, Dependency) || !Dependency.mExists || Dependency.mSize > MaxFileSize) {
        Diagnostics = "Cannot read shader file: " + FilePath::ToGenericUtf8(Path);
        return false;
    }

    std::ifstream Stream{Path, std::ios::binary};

    Data.resize(static_cast<std::size_t>(Dependency.mSize));

    if (!Stream || (!Data.empty() && !Stream.read(reinterpret_cast<char*>(Data.data()), static_cast<std::streamsize>(Data.size())))) {
        Diagnostics = "Cannot read shader file: " + FilePath::ToGenericUtf8(Path);
        return false;
    }

    FShaderDependency Current{};

    if (!Inspect(Path, Current) || !Current.mExists || Current.mWriteTime != Dependency.mWriteTime || Current.mSize != Dependency.mSize) {
        Diagnostics = "Shader file changed while reading: " + FilePath::ToGenericUtf8(Path);
        return false;
    }

    return true;
}

bool ShaderFile::IsSnapshotCurrent(const TArray<FShaderDependency>& Dependencies) {
    for (const FShaderDependency& Dependency : Dependencies) {
        FShaderDependency Current{};

        if (!Inspect(Dependency.mPath, Current) || Current.mExists != Dependency.mExists || (Current.mExists && (Current.mWriteTime != Dependency.mWriteTime || Current.mSize != Dependency.mSize))) {
            return false;
        }
    }

    return true;
}

FString ShaderFile::Hash(std::span<const Uint8> Data) {
    if (Data.size() > std::numeric_limits<ULONG>::max()) {
        return {};
    }

    BCRYPT_ALG_HANDLE Algorithm{};

    if (BCryptOpenAlgorithmProvider(&Algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) {
        return {};
    }

    std::array<Uint8, 32> Digest{};
    const NTSTATUS Status{BCryptHash(Algorithm, nullptr, 0, const_cast<PUCHAR>(Data.data()), static_cast<ULONG>(Data.size()), Digest.data(), static_cast<ULONG>(Digest.size()))};

    BCryptCloseAlgorithmProvider(Algorithm, 0);

    if (Status < 0) {
        return {};
    }

    constexpr char Digits[]{"0123456789abcdef"};
    FString Result{};
    Result.reserve(Digest.size() * 2);

    for (const Uint8 Byte : Digest) {
        Result.push_back(Digits[Byte >> 4]);
        Result.push_back(Digits[Byte & 15]);
    }

    return Result;
}

const FString& ShaderFile::GetCompilerIdentity() {
    static const FString Identity{MakeCompilerIdentity()};
    return Identity;
}

void ShaderFile::AppendUint64(TArray<Uint8>& Data, Uint64 Value) {
    for (Uint32 Shift{}; Shift < 64; Shift += 8) {
        Data.push_back(static_cast<Uint8>(Value >> Shift));
    }
}

bool ShaderFile::ReadUint64(std::span<const Uint8> Data, std::size_t& Position, Uint64& Value) {
    if (Position > Data.size() || Data.size() - Position < sizeof(Uint64)) {
        return false;
    }

    Value = 0;

    for (Uint32 Shift{}; Shift < 64; Shift += 8) {
        Value |= static_cast<Uint64>(Data[Position++]) << Shift;
    }

    return true;
}

void ShaderFile::AppendString(TArray<Uint8>& Data, std::string_view Value) {
    AppendUint64(Data, Value.size());
    Data.insert(Data.end(), Value.begin(), Value.end());
}

bool ShaderFile::ReadString(std::span<const Uint8> Data, std::size_t& Position, FString& Value) {
    Uint64 Size{};

    if (!ReadUint64(Data, Position, Size) || Size > 1024 * 1024 || Size > Data.size() - Position) {
        return false;
    }

    Value.assign(reinterpret_cast<const char*>(Data.data() + Position), static_cast<std::size_t>(Size));
    Position += static_cast<std::size_t>(Size);

    return true;
}
