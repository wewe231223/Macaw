/**
 * @file FMeshImportTexture.h
 * @brief 원본 재질 텍스처가 없는지, 외부 파일인지, 내장 참조인지 구분하는 값 객체입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

#include <filesystem>

namespace Asset {

    /**
     * @brief 임포트한 재질 텍스처의 없음·외부 파일·내장 참조 출처를 구분합니다.
     */
    enum class EMeshImportTextureSource : Core::Uint8 {
        /**
         * @brief 원본 재질에 해당 텍스처 참조가 없습니다.
         */
        None,
        /**
         * @brief 원본 파일의 외부 이미지 경로를 참조합니다.
         */
        ExternalFile,
        /**
         * @brief 원본 장면 내부의 텍스처를 참조합니다.
         */
        Embedded
    };

    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectEnum(EMeshImportTextureSource, None, ExternalFile, Embedded);

    /**
     * @fn UEnum* Asset::GetNativeEnum(TNativeEnum<EMeshImportTextureSource>)
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
     */

    /**
     * @brief 원본 재질 텍스처가 없는지, 외부 파일인지, 내장 참조인지 구분하는 값 객체입니다.
     */
    class FMeshImportTexture {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FMeshImportTexture);

    public:
        /**
         * @brief 기본 설정으로 FMeshImportTexture 객체를 초기화합니다.
         */
        FMeshImportTexture() = default;

        /**
         * @brief 전달받은 설정을 보관하도록 FMeshImportTexture 객체를 초기화합니다.
         * @param[in] FilePath 외부 텍스처 파일의 경로입니다.
         */
        explicit FMeshImportTexture(std::filesystem::path FilePath);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FMeshImportTexture);

        /**
         * @brief 원본 장면 내부 텍스처의 이름 또는 인덱스를 보관하는 참조를 생성합니다.
         * @param[in] Reference 원본 장면 안 텍스처의 이름 또는 인덱스 참조입니다.
         * @param[in] Data 원본에서 추출한 내장 텍스처의 인코딩된 바이트이며 비어 있으면 참조만 보관합니다.
         * @return Embedded 출처와 지정한 내장 참조를 가진 값 객체입니다.
         */
        static Asset::FMeshImportTexture FromEmbedded(Core::FString Reference, Core::TArray<Core::Uint8> Data = {});

        /**
         * @brief 재질 텍스처의 출처 종류를 반환합니다.
         * @return 텍스처 없음·외부 파일·내장 참조 중 하나입니다.
         */
        Asset::EMeshImportTextureSource GetSource() const;
        /**
         * @brief 외부 텍스처 파일 경로를 조회합니다.
         * @return 보관 중인 파일 경로의 읽기 전용 참조이며 외부 파일이 아니면 기본값은 빈 경로입니다.
         */
        const std::filesystem::path& GetFilePath() const;
        /**
         * @brief 원본 장면 내부의 텍스처 식별 문자열을 조회합니다.
         * @return 내장 텍스처 참조의 읽기 전용 문자열이며 기본값은 빈 문자열입니다.
         */
        const Core::FString& GetEmbeddedReference() const;

        /**
         * @brief 원본에서 추출한 내장 텍스처 바이트를 조회합니다.
         * @return 인코딩된 이미지 바이트의 읽기 전용 참조이며 추출한 데이터가 없으면 빈 배열입니다.
         */
        const Core::TArray<Core::Uint8>& GetEmbeddedData() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 텍스처 없음·외부 파일·내장 참조 중 텍스처의 출처입니다.
         */
        Asset::EMeshImportTextureSource mSource{Asset::EMeshImportTextureSource::None};
        ReflectProperty(mSource, "Source", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 외부 텍스처 파일을 읽을 파일 시스템 경로입니다.
         */
        std::filesystem::path mFilePath{};
        ReflectProperty(mFilePath, "FilePath", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 원본 장면 안에 포함된 텍스처의 이름 또는 인덱스 참조입니다.
         */
        Core::FString mEmbeddedReference{};
        ReflectProperty(mEmbeddedReference, "EmbeddedReference", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 원본에서 추출한 내장 텍스처의 인코딩된 이미지 바이트입니다.
         */
        Core::TArray<Core::Uint8> mEmbeddedData{};
        ReflectProperty(mEmbeddedData, "EmbeddedData", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
