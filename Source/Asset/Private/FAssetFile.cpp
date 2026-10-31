/**
 * @file FAssetFile.cpp
 * @brief 에셋 파일의 종류, 버전, 무결성 검사와 바이너리 입출력에 대한 구현입니다.
 * @ingroup Asset
 */

#include "Asset/FAssetFile.h"
#include "Core/FAtomicFileWriter.h"
#include "Core/LoadingStatus.h"

#include <array>
#include <fstream>

namespace Asset {

    /**
     * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
     */
    namespace {
        /**
         * @brief Macaw 에셋 파일을 식별하는 헤더의 매직 바이트입니다.
         */
        constexpr std::array<char, 8> AssetMagic{'M', 'A', 'C', 'A', 'W', 'A', 'S', 'T'};

        /**
         * @brief 현재 지원하는 에셋 파일 형식 버전입니다.
         */
        constexpr Core::Uint32 AssetVersion{1};

        /**
         * @brief 에셋 파일에서 허용하는 최대 페이로드 바이트 수입니다.
         */
        constexpr Core::Uint64 MaximumPayload{512ULL * 1024 * 1024};

        /**
         * @brief 페이로드 앞에 저장하는 에셋 헤더의 바이트 수입니다.
         */
        constexpr Core::Uint64 HeaderSize{24};

        /**
         * @brief 에셋 파일을 열고 매직·버전·종류·데이터 크기를 검증합니다.
         * @param[in] Path 열 에셋 파일의 경로입니다.
         * @param[in,out] Input 검증할 에셋 파일을 열고 헤더를 읽을 입력 스트림입니다.
         * @param[out] Type 에셋 파일에 기록하거나 검사할 종류입니다.
         * @param[out] Size 처리할 데이터의 바이트 수입니다.
         * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool ReadHeader(const std::filesystem::path& Path, std::ifstream& Input, Asset::EAssetType& Type, Core::Uint64& Size, Core::FString& Error) {
            Error.clear();

            std::error_code Code{};
            const Core::Uint64 FileSize{std::filesystem::file_size(Path, Code)};

            if (Code || Path.extension() != L".masset" || FileSize < HeaderSize || FileSize - HeaderSize > MaximumPayload) {
                Error = "Cannot read asset file or invalid asset size";
                return false;
            }

            Input.open(Path, std::ios::binary);

            std::array<char, 8> Magic{};
            Core::Uint32 Version{};
            Core::Uint32 TypeValue{};
            Input.read(Magic.data(), Magic.size());
            Input.read(reinterpret_cast<char*>(&Version), sizeof(Version));
            Input.read(reinterpret_cast<char*>(&TypeValue), sizeof(TypeValue));
            Input.read(reinterpret_cast<char*>(&Size), sizeof(Size));

            if (!Input || Magic != AssetMagic || Version != AssetVersion || TypeValue < static_cast<Core::Uint32>(Asset::EAssetType::Scene) || TypeValue > static_cast<Core::Uint32>(Asset::EAssetType::Material) || Size == 0 || Size != FileSize - HeaderSize) {
                Error = "Invalid asset header, version or type";
                return false;
            }

            Type = static_cast<Asset::EAssetType>(TypeValue);

            return true;
        }
    }

    bool Asset::FAssetFile::GetType(const std::filesystem::path& Path, Asset::EAssetType& Type, Core::FString& Error) {
        std::ifstream Input{};
        Core::Uint64 Size{};

        return ReadHeader(Path, Input, Type, Size, Error);
    }

    bool Asset::FAssetFile::Save(const std::filesystem::path& Path, Asset::EAssetType Type, std::span<const Core::Uint8> Payload, Core::FString& Error) {
        Error.clear();

        if (Path.extension() != L".masset" || Payload.empty() || Payload.size() > MaximumPayload || Type < Asset::EAssetType::Scene || Type > Asset::EAssetType::Material) {
            Error = "Invalid asset path, type or payload";
            return false;
        }

        std::error_code Code{};
        const std::filesystem::path Target{std::filesystem::absolute(Path, Code)};

        if (Code) {
            Error = "Invalid asset path: " + Code.message();
            return false;
        }

        Asset::EAssetType ExistingType{Type};
        Core::FString ExistingError{};

        if (GetType(Target, ExistingType, ExistingError) && ExistingType != Type) {
            Error = "Cannot overwrite an asset with a different asset type";
            return false;
        }

        Core::FAtomicFileWriter Writer{};

        if (!Writer.Open(Target, Error)) {
            return false;
        }

        const Core::Uint32 TypeValue{static_cast<Core::Uint32>(Type)};
        const Core::Uint64 Size{Payload.size()};
        std::ostream& Output{Writer.GetStream()};
        Output.write(AssetMagic.data(), AssetMagic.size());
        Output.write(reinterpret_cast<const char*>(&AssetVersion), sizeof(AssetVersion));
        Output.write(reinterpret_cast<const char*>(&TypeValue), sizeof(TypeValue));
        Output.write(reinterpret_cast<const char*>(&Size), sizeof(Size));
        Output.write(reinterpret_cast<const char*>(Payload.data()), static_cast<std::streamsize>(Size));

        return Writer.Commit(Error);
    }

    bool Asset::FAssetFile::Load(const std::filesystem::path& Path, Asset::EAssetType Type, Core::TArray<Core::Uint8>& Payload, Core::FString& Error) {
        Core::LoadingStatus::ReportFile("Loading", Path);

        std::ifstream Input{};
        Asset::EAssetType ActualType{Type};
        Core::Uint64 Size{};

        if (!ReadHeader(Path, Input, ActualType, Size, Error)) {
            return false;
        }

        if (ActualType != Type) {
            Error = "The asset type does not match the requested type";
            return false;
        }

        Core::TArray<Core::Uint8> Candidate{};
        Candidate.resize(static_cast<std::size_t>(Size));
        Input.read(reinterpret_cast<char*>(Candidate.data()), static_cast<std::streamsize>(Size));

        if (!Input) {
            Error = "Truncated asset payload";
            return false;
        }

        Payload = std::move(Candidate);

        return true;
    }

}
