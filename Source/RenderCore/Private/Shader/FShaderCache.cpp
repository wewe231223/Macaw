/**
 * @file FShaderCache.cpp
 * @brief 컴파일된 셰이더의 디스크 캐시와 의존성 유효성 검사에 대한 구현입니다.
 * @ingroup RenderCore
 */

#include "RenderCore/Shader/FShaderCache.h"
#include "Core/FilePath.h"
#include "FShaderFile.h"

#include <Windows.h>

#include <atomic>
#include <bit>
#include <utility>

namespace RenderCore {

    /**
     * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
     */
    namespace {
        /**
         * @brief 셰이더 캐시 파일에서 지원하는 형식 버전입니다.
         */
        constexpr Core::Uint64 FormatVersion{1};

        /**
         * @brief 셰이더 캐시에 저장할 수 있는 최대 의존 파일 개수입니다.
         */
        constexpr Core::Uint64 MaxDependencies{4096};

        /**
         * @brief 16진수 SHA-256 체크섬의 바이트 수입니다.
         */
        constexpr std::size_t ChecksumSize{64};

        /**
         * @brief 파일 수정 시각을 캐시 직렬화에 사용하는 64비트 값으로 변환합니다.
         * @param[in] Time 캐시 형식으로 인코딩하거나 복원할 파일 시각입니다.
         * @return 변환 또는 계산된 부호 없는 정수 값입니다.
         */
        Core::Uint64 EncodeTime(std::filesystem::file_time_type Time) {
            return std::bit_cast<Core::Uint64>(static_cast<std::int64_t>(Time.time_since_epoch().count()));
        }

        /**
         * @brief 캐시에 저장한 64비트 값을 파일 수정 시각으로 복원합니다.
         * @param[in] Time 캐시 형식으로 인코딩하거나 복원할 파일 시각입니다.
         * @return 요청하거나 변환한 파일 시스템 경로 또는 시각입니다.
         */
        std::filesystem::file_time_type DecodeTime(Core::Uint64 Time) {
            return std::filesystem::file_time_type{std::filesystem::file_time_type::duration{std::bit_cast<std::int64_t>(Time)}};
        }

        /**
         * @brief 캐시 헤더와 셰이더 식별자를 검증하여 컴파일 결과를 복원합니다.
         * @param[in] Data 처리할 바이트 데이터 또는 include 파일 버퍼입니다.
         * @param[in] ExpectedIdentity 캐시에 저장된 값과 일치해야 하는 셰이더 식별 문자열입니다.
         * @param[out] Compilation 셰이더 바이트코드와 의존성을 포함하는 컴파일 결과입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool ReadCompilation(std::span<const Core::Uint8> Data, const Core::FString& ExpectedIdentity, RenderCore::FShaderCompilation& Compilation) {
            std::size_t Position{};
            Core::FString Magic{};
            Core::FString Identity{};
            Core::Uint64 Version{};
            Core::Uint64 BuildTime{};
            Core::Uint64 Count{};

            if (!RenderCore::ShaderFile::ReadString(Data, Position, Magic) || Magic != "Macaw.Shader" || !RenderCore::ShaderFile::ReadUint64(Data, Position, Version) || Version != FormatVersion || !RenderCore::ShaderFile::ReadString(Data, Position, Identity) || Identity != ExpectedIdentity || !RenderCore::ShaderFile::ReadUint64(Data, Position, BuildTime) || !RenderCore::ShaderFile::ReadUint64(Data, Position, Count) || Count == 0 || Count > MaxDependencies) {
                return false;
            }

            RenderCore::FShaderCompilation Candidate{};
            Candidate.mBuildTime = DecodeTime(BuildTime);
            Candidate.mDependencies.reserve(static_cast<std::size_t>(Count));

            for (Core::Uint64 Index{}; Index < Count; ++Index) {
                Core::FString Path{};
                Core::Uint64 Exists{};
                Core::Uint64 WriteTime{};
                RenderCore::FShaderDependency Dependency{};

                if (!RenderCore::ShaderFile::ReadString(Data, Position, Path) || Path.empty() || Path.find('\0') != Core::FString::npos || !RenderCore::ShaderFile::ReadUint64(Data, Position, Exists) || Exists > 1 || !RenderCore::ShaderFile::ReadUint64(Data, Position, WriteTime) || !RenderCore::ShaderFile::ReadUint64(Data, Position, Dependency.mSize)) {
                    return false;
                }

                Dependency.mPath = Core::FilePath::FromUtf8(Path);
                Dependency.mExists = Exists != 0;
                Dependency.mWriteTime = DecodeTime(WriteTime);
                Candidate.mDependencies.push_back(std::move(Dependency));
            }

            Core::Uint64 ByteCodeSize{};

            if (!RenderCore::ShaderFile::ReadUint64(Data, Position, ByteCodeSize) || ByteCodeSize == 0 || ByteCodeSize != Data.size() - Position) {
                return false;
            }

            Candidate.mByteCode.assign(Data.begin() + Position, Data.end());
            Compilation = std::move(Candidate);

            return true;
        }
    }

    RenderCore::FShaderCache::FShaderCache(std::filesystem::path Directory)
		: mDirectory{std::move(Directory)} {
    }

    std::filesystem::path RenderCore::FShaderCache::GetPath(const Core::FString& CacheKey) const {
        return mDirectory / (CacheKey + ".Shader");
    }

    bool RenderCore::FShaderCache::Load(const RenderCore::FShaderDescription& Description, RenderCore::FShaderCompilation& Compilation, Core::FString& Diagnostics) const {
        RenderCore::FShaderDescription Normalized{Description};

        if (!Normalized.Normalize(Diagnostics)) {
            return false;
        }

        const Core::FString Key{Normalized.GetCacheKey()};

        if (Key.empty()) {
            Diagnostics = "Cannot compute shader cache key";
            return false;
        }

        const std::filesystem::path Path{GetPath(Key)};
        Core::TArray<Core::Uint8> Data{};
        RenderCore::FShaderDependency Dependency{};

        if (!RenderCore::ShaderFile::Read(Path, Data, Dependency, Diagnostics)) {
            return false;
        }

        if (Data.size() <= ChecksumSize) {
            Diagnostics = "Invalid shader cache: " + Core::FilePath::ToGenericUtf8(Path);
            return false;
        }

        const std::span<const Core::Uint8> Payload{Data.data() + ChecksumSize, Data.size() - ChecksumSize};
        const Core::FString ExpectedChecksum{reinterpret_cast<const char*>(Data.data()), ChecksumSize};
        RenderCore::FShaderCompilation Candidate{};

        if (RenderCore::ShaderFile::Hash(Payload) != ExpectedChecksum || !ReadCompilation(Payload, Normalized.GetIdentity(), Candidate) || !IsCurrent(Candidate)) {
            Diagnostics = "Invalid or outdated shader cache: " + Core::FilePath::ToGenericUtf8(Path);
            return false;
        }

        Compilation = std::move(Candidate);
        Diagnostics.clear();

        return true;
    }

    bool RenderCore::FShaderCache::Save(const RenderCore::FShaderDescription& Description, const RenderCore::FShaderCompilation& Compilation, Core::FString& Diagnostics) const {
        RenderCore::FShaderDescription Normalized{Description};

        if (!Normalized.Normalize(Diagnostics)) {
            return false;
        }

        if (Compilation.mByteCode.empty() || Compilation.mDependencies.empty() || Compilation.mDependencies.size() > MaxDependencies) {
            Diagnostics = "Cannot cache an empty or oversized shader compilation";
            return false;
        }

        const Core::FString Key{Normalized.GetCacheKey()};

        if (Key.empty()) {
            Diagnostics = "Cannot compute shader cache key";
            return false;
        }

        Core::TArray<Core::Uint8> Payload{};
        RenderCore::ShaderFile::AppendString(Payload, "Macaw.Shader");
        RenderCore::ShaderFile::AppendUint64(Payload, FormatVersion);
        RenderCore::ShaderFile::AppendString(Payload, Normalized.GetIdentity());
        RenderCore::ShaderFile::AppendUint64(Payload, EncodeTime(Compilation.mBuildTime));
        RenderCore::ShaderFile::AppendUint64(Payload, Compilation.mDependencies.size());

        for (const RenderCore::FShaderDependency& Dependency : Compilation.mDependencies) {
            RenderCore::ShaderFile::AppendString(Payload, Core::FilePath::ToGenericUtf8(Dependency.mPath));
            RenderCore::ShaderFile::AppendUint64(Payload, Dependency.mExists ? 1 : 0);
            RenderCore::ShaderFile::AppendUint64(Payload, EncodeTime(Dependency.mWriteTime));
            RenderCore::ShaderFile::AppendUint64(Payload, Dependency.mSize);
        }

        RenderCore::ShaderFile::AppendUint64(Payload, Compilation.mByteCode.size());
        Payload.insert(Payload.end(), Compilation.mByteCode.begin(), Compilation.mByteCode.end());

        const Core::FString Checksum{RenderCore::ShaderFile::Hash(Payload)};

        if (Checksum.size() != ChecksumSize || Payload.size() > 64 * 1024 * 1024 - ChecksumSize) {
            Diagnostics = "Cannot serialize shader cache";
            return false;
        }

        std::error_code Error{};
        std::filesystem::create_directories(mDirectory, Error);

        if (Error) {
            Diagnostics = "Cannot create shader cache directory: " + Core::FilePath::ToGenericUtf8(mDirectory);
            return false;
        }

        const std::filesystem::path Path{GetPath(Key)};
        static std::atomic<Core::Uint64> Sequence{};
        std::filesystem::path Temporary{Path};
        Temporary += L"." + std::to_wstring(GetCurrentProcessId()) + L"." + std::to_wstring(GetTickCount64()) + L"." + std::to_wstring(Sequence.fetch_add(1)) + L".tmp";

        const HANDLE File{CreateFileW(Temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr)};

        if (File == INVALID_HANDLE_VALUE) {
            Diagnostics = "Cannot create shader cache: " + Core::FilePath::ToGenericUtf8(Temporary);
            return false;
        }

        DWORD Written{};
        bool Success{WriteFile(File, Checksum.data(), static_cast<DWORD>(Checksum.size()), &Written, nullptr) != FALSE && Written == Checksum.size()};
        Success = Success && WriteFile(File, Payload.data(), static_cast<DWORD>(Payload.size()), &Written, nullptr) != FALSE && Written == Payload.size();
        Success = Success && FlushFileBuffers(File) != FALSE;

        const bool Closed{CloseHandle(File) != FALSE};

        if (!Success || !Closed || !MoveFileExW(Temporary.c_str(), Path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            DeleteFileW(Temporary.c_str());
            Diagnostics = "Cannot replace shader cache: " + Core::FilePath::ToGenericUtf8(Path);
            return false;
        }

        Diagnostics.clear();

        return true;
    }

    bool RenderCore::FShaderCache::IsCurrent(const RenderCore::FShaderCompilation& Compilation) const {
        if (Compilation.mByteCode.empty() || Compilation.mDependencies.empty()) {
            return false;
        }

        for (const RenderCore::FShaderDependency& Dependency : Compilation.mDependencies) {
            RenderCore::FShaderDependency Current{};

            if (!RenderCore::ShaderFile::Inspect(Dependency.mPath, Current) || Current.mExists != Dependency.mExists || (Current.mExists && Current.mWriteTime > Compilation.mBuildTime)) {
                return false;
            }
        }

        return true;
    }

}
