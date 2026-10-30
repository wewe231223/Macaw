/**
 * @file IMeshImporter.h
 * @brief 원본 파일에서 메시 데이터와 재질 참조를 가져오는 임포터의 공통 인터페이스입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "Asset/Import/FMeshImportResult.h"

#include <filesystem>
#include <memory>
#include <string_view>
#include <utility>

namespace Asset {

    /**
     * @brief 원본 파일에서 메시 데이터와 재질 참조를 가져오는 임포터의 공통 인터페이스입니다.
     */
    class IMeshImporter {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(IMeshImporter);

    private:
        /**
         * @brief 원본 경로로 구체 메시 임포터를 생성하는 팩토리 함수 포인터입니다.
         */
        using FImporterFactory = std::unique_ptr<Asset::IMeshImporter> (*)(const std::filesystem::path&);
        /**
         * @brief 소문자 확장자와 그 확장자를 처리하는 임포터 팩토리의 쌍입니다.
         */
        using FImporterEntry = std::pair<std::wstring_view, FImporterFactory>;

    public:
        /**
         * @brief 가져올 원본 파일 경로를 보관하도록 임포터를 초기화합니다.
         * @param[in] Source 가져올 원본 메시 파일 경로입니다.
         */
        explicit IMeshImporter(std::filesystem::path Source);

        /**
         * @brief 객체 수명을 끝내고 소유한 자원을 정리합니다.
         */
        virtual ~IMeshImporter();

        IMeshImporter(const Asset::IMeshImporter&) = delete;
        Asset::IMeshImporter& operator=(const Asset::IMeshImporter&) = delete;
        IMeshImporter(Asset::IMeshImporter&&) = delete;
        Asset::IMeshImporter& operator=(Asset::IMeshImporter&&) = delete;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(IMeshImporter);

        /**
         * @brief 원본 파일 확장자에 맞는 메시 임포터를 생성합니다.
         * @param[in] Source 가져올 원본 메시 파일 경로입니다.
         * @return 확장자에 등록된 임포터이며 등록되지 않은 확장자이면 Assimp 임포터입니다.
         */
        static std::unique_ptr<Asset::IMeshImporter> CreateForSource(const std::filesystem::path& Source);

        /**
         * @brief 원본 파일 확장자에 등록된 메시 임포터가 있는지 확인합니다.
         * @param[in] Source 확인할 원본 메시 파일 경로입니다.
         * @return 등록된 확장자이면 true입니다.
         */
        static bool IsSupported(const std::filesystem::path& Source);

        /**
         * @brief 원본 메시 파일을 읽어 정점·삼각형·재질 슬롯과 텍스처 참조를 수집하고 경계 상자를 계산합니다.
         * @param[out] Result 가져온 메시 데이터와 재질 목록을 기록할 결과 객체입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 유효한 임포트 결과를 구성하면 true이며 실패하면 Error에 원인을 기록하고 false입니다.
         */
        bool Import(Asset::FMeshImportResult& Result, Core::FString& Error);

    protected:
        /**
         * @brief 가져오는 원본 파일 경로를 조회합니다.
         * @return Import 시작 후에는 절대 경로로 정규화된 원본 경로의 읽기 전용 참조입니다.
         */
        const std::filesystem::path& GetSourcePath() const;
        /**
         * @brief 원본 파일이 위치한 디렉터리를 조회합니다.
         * @return 상대 텍스처 경로를 해석할 기준 디렉터리의 읽기 전용 참조입니다.
         */
        const std::filesystem::path& GetSourceDirectory() const;
        /**
         * @brief 파생 임포터가 채울 임포트 결과를 조회합니다.
         * @return 메시 데이터와 재질 목록을 기록할 결과 객체의 참조입니다.
         */
        Asset::FMeshImportResult& GetResult();
        /**
         * @brief 파생 임포터가 실패 원인을 기록할 문자열을 조회합니다.
         * @return Import 실패 시 호출자에게 전달할 오류 문자열의 참조입니다.
         */
        Core::FString& GetError();

    private:
        /**
         * @brief 원본 파일 확장자에 등록된 임포터 팩토리를 찾습니다.
         * @param[in] Source 확장자를 확인할 원본 메시 파일 경로입니다.
         * @return 등록된 팩토리이며 지원하지 않는 확장자이면 nullptr입니다.
         */
        static FImporterFactory FindFactory(const std::filesystem::path& Source);
        /**
         * @brief Assimp 기반 메시 임포터를 생성합니다.
         * @param[in] Source 가져올 원본 메시 파일 경로입니다.
         * @return 새로 생성한 FAssimpMeshImporter입니다.
         */
        static std::unique_ptr<Asset::IMeshImporter> CreateAssimp(const std::filesystem::path& Source);
        /**
         * @brief FBX SDK 기반 메시 임포터를 생성합니다.
         * @param[in] Source 가져올 원본 FBX 파일 경로입니다.
         * @return 새로 생성한 FFBXLoader입니다.
         */
        static std::unique_ptr<Asset::IMeshImporter> CreateFBX(const std::filesystem::path& Source);

        /**
         * @brief 파생 임포터가 원본 파일을 읽어 GetResult()의 메시 데이터와 재질 목록을 채웁니다.
         * @return 성공하면 true이며 실패하면 GetError()에 원인을 기록하고 false입니다.
         */
        virtual bool ImportSource() = 0;

        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 가져올 원본 메시 파일 경로입니다.
         */
        std::filesystem::path mSourcePath{};
        ReflectProperty(mSourcePath, "SourcePath", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 상대 텍스처 경로를 해석할 원본 파일의 디렉터리입니다.
         */
        std::filesystem::path mSourceDirectory{};
        ReflectProperty(mSourceDirectory, "SourceDirectory", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 파생 임포터가 채우는 메시 데이터와 재질 목록입니다.
         */
        Asset::FMeshImportResult mResult{};
        ReflectProperty(mResult, "Result", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 파생 임포터가 기록한 실패 원인입니다.
         */
        Core::FString mError{};
        ReflectProperty(mError, "Error", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 임포터를 한 번만 사용하도록 Import 호출 여부를 기록합니다.
         */
        bool mImportStarted{};
        ReflectProperty(mImportStarted, "ImportStarted", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
