/**
 * @file FFBXImageProcessor.h
 * @brief FBX 장면의 파일 텍스처를 내장 바이트나 외부 파일에서 찾아 형태를 판별하고 재질 슬롯 정책을 적용합니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "Asset/Import/FMeshImportTexture.h"

#include <fbxsdk.h>
#include <filesystem>
#include <span>

namespace DirectX {

    struct TexMetadata;

}

namespace Asset {

    /**
     * @brief FBX 장면의 파일 텍스처를 내장 바이트나 외부 파일에서 찾아 형태를 판별하고 재질 슬롯 정책을 적용합니다.
     * @details 내장 바이트 수집, Import 이후 일괄 판별, 재질 속성별 조회의 세 단계를 거치며 픽셀 디코딩은 하지 않습니다.
     */
    class FFBXImageProcessor final {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FFBXImageProcessor);

    private:
        /**
         * @brief 정규화한 텍스처 경로 키와 FBX에 내장된 원본 바이트의 대응입니다.
         */
        using FEmbeddedData = Core::TMap<Core::FString, Core::TArray<Core::Uint8>>;
        /**
         * @brief FBX 파일 텍스처와 형태를 판별한 임포트 텍스처의 대응입니다.
         */
        using FResolvedTextures = Core::TMap<fbxsdk::FbxFileTexture*, Asset::FMeshImportTexture>;
        /**
         * @brief FBX 파일 텍스처와 판별에 실패한 원인의 대응입니다.
         */
        using FTextureErrors = Core::TMap<fbxsdk::FbxFileTexture*, Core::FString>;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FFBXImageProcessor);

    public:
        /**
         * @brief Import 중 FBX SDK가 만나는 내장 미디어 바이트를 이 처리기에 보관하도록 임포터에 콜백을 등록합니다.
         * @details 콜백은 이 처리기의 주소를 사용자 데이터로 받으므로 Import가 끝날 때까지 처리기를 옮기거나 해제하면 안 됩니다.
         * @param[in] Manager 콜백 객체를 소유할 FBX SDK 관리자입니다.
         * @param[in,out] Importer 콜백을 등록할 FBX 임포터입니다.
         */
        void RegisterEmbeddedCallback(fbxsdk::FbxManager& Manager, fbxsdk::FbxImporter& Importer);
        /**
         * @brief Import 이후 장면의 모든 파일 텍스처를 판별해 결과 또는 오류를 기록하고 보관한 내장 바이트를 해제합니다.
         * @param[in] Scene Import를 마친 FBX 장면입니다.
         * @param[in] SourceDirectory 외부 텍스처 파일을 찾을 원본 FBX 파일의 디렉터리입니다.
         */
        void ResolveTextures(const fbxsdk::FbxScene& Scene, const std::filesystem::path& SourceDirectory);
        /**
         * @brief 재질 속성에 연결된 파일 텍스처의 판별 결과를 조회하고 재질 슬롯이 받아들이는 형태인지 확인합니다.
         * @param[in] Property 텍스처가 연결된 FBX 재질 속성입니다.
         * @param[in] Accepted 이 재질 슬롯이 받아들이는 텍스처 형태 목록입니다.
         * @param[out] Result 판별한 텍스처 참조를 기록할 대상이며 연결된 텍스처가 없으면 바꾸지 않습니다.
         * @param[out] Error 실패 원인을 기록할 대상입니다.
         * @return 텍스처를 받아들였거나 연결되지 않았으면 true이며 레이어드 텍스처이거나 판별에 실패했거나 받아들이지 않는 형태이면 false입니다.
         */
        bool ReadTexture(fbxsdk::FbxProperty Property, std::span<const Asset::EMeshImportTextureShape> Accepted, Asset::FMeshImportTexture& Result, Core::FString& Error) const;

    private:
        /**
         * @brief 텍스처 경로를 구분자와 대소문자에 무관하게 비교할 수 있는 키로 정규화합니다.
         * @param[in] Path 정규화할 UTF-8 텍스처 경로입니다.
         * @return 슬래시 구분자와 소문자로 정규화한 UTF-8 경로입니다.
         */
        Core::FString TextureKey(Core::FString Path) const;
        /**
         * @brief FBX SDK가 넘겨준 내장 미디어 바이트를 판별 없이 mEmbeddedData에 보관합니다.
         * @details 콜백 시점에는 장면 연결과 FbxVideo 속성을 알 수 없으므로 판별과 필터링은 ResolveTextures가 맡습니다.
         * @param[in] DataHint 내장 데이터를 사용할 FBX 객체의 클래스 식별자입니다.
         * @param[in] FileName FBX에 기록된 내장 파일 이름입니다.
         * @param[in] Buffer 내장 파일 바이트의 비소유 주소입니다.
         * @param[in] Size Buffer의 바이트 수입니다.
         * @return 처리하거나 제외한 데이터는 eHandled이며 무효한 인수는 eFailure입니다.
         */
        fbxsdk::FbxCallback::State StoreEmbeddedData(fbxsdk::FbxClassId DataHint, const char* FileName, const void* Buffer, std::size_t Size);

        /**
         * @brief 오류 메시지에 쓸 텍스처 형태 이름을 반환합니다.
         * @param[in] Shape 이름을 구할 텍스처 형태입니다.
         * @return 형태의 정적 문자열입니다.
         */
        const char* ShapeName(Asset::EMeshImportTextureShape Shape) const;
        /**
         * @brief PNG 청크 헤더만 따라가며 APNG 애니메이션 제어 청크가 있는지 확인합니다.
         * @details WIC는 APNG의 기본 이미지만 단일 프레임으로 읽으므로 애니메이션 여부를 따로 확인해야 합니다.
         * @param[in] Data 내장 텍스처의 인코딩된 바이트이며 비어 있으면 Path의 외부 파일에서 청크 헤더만 읽습니다.
         * @param[in] Path Data가 비어 있으면 읽을 외부 파일 경로입니다.
         * @return 첫 IDAT 앞에 acTL 청크가 있으면 true이며 PNG가 아니거나 읽을 수 없으면 false입니다.
         */
        bool HasPNGAnimation(std::span<const Core::Uint8> Data, const std::filesystem::path& Path) const;
        /**
         * @brief DirectXTex 메타데이터의 차원·큐브맵·배열 정보로 텍스처 형태를 정하고 형태별 D3D11 크기 한계를 확인합니다.
         * @param[in] Metadata 이미지 헤더에서 읽은 메타데이터입니다.
         * @param[in] Name 오류 메시지에 쓸 이미지 이름입니다.
         * @param[out] Shape 판별한 텍스처 형태입니다.
         * @param[out] Error 지원하지 않는 차원이거나 크기 한계를 넘으면 원인을 기록할 대상입니다.
         * @return 형태를 정했고 크기가 한계 안이면 true입니다.
         */
        bool ClassifyMetadata(const DirectX::TexMetadata& Metadata, const Core::FString& Name, Asset::EMeshImportTextureShape& Shape, Core::FString& Error) const;
        /**
         * @brief DirectXTex가 직접 읽지 않는 형식을 WIC 디코더로 열어 프레임 수와 첫 프레임 크기로 텍스처 형태를 판별합니다.
         * @details 외부 파일은 파일 이름으로 디코더를 열어 WIC가 필요한 헤더만 읽게 하며 픽셀은 복사하지 않습니다.
         * @param[in] Data 내장 텍스처의 인코딩된 바이트이며 비어 있으면 Path의 외부 파일에서 읽습니다.
         * @param[in] Path 확장자와 오류 메시지에 쓸 경로이며 Data가 비어 있으면 읽을 외부 파일 경로입니다.
         * @param[out] Shape 판별한 텍스처 형태입니다.
         * @param[out] Error 형식을 인식할 수 없으면 원인을 기록할 대상입니다.
         * @return 디코더를 열고 첫 프레임 크기가 텍스처 한계 안이면 true입니다.
         */
        bool InspectWICImage(std::span<const Core::Uint8> Data, const std::filesystem::path& Path, Asset::EMeshImportTextureShape& Shape, Core::FString& Error) const;
        /**
         * @brief 이미지 헤더 메타데이터만 읽어 픽셀 디코딩 없이 텍스처 형태를 판별합니다.
         * @details 픽셀 디코딩은 FAssetManager가 텍스처 에셋을 만들 때 수행합니다.
         * @param[in] Data 내장 텍스처의 인코딩된 바이트이며 비어 있으면 Path의 외부 파일에서 읽습니다.
         * @param[in] Path 확장자와 오류 메시지에 쓸 경로이며 Data가 비어 있으면 읽을 외부 파일 경로입니다.
         * @param[out] Shape 판별한 텍스처 형태입니다.
         * @param[out] Error 형식을 인식할 수 없으면 원인을 기록할 대상입니다.
         * @return 헤더를 읽을 수 있고 크기가 텍스처 한계 안이면 true입니다.
         */
        bool InspectImage(std::span<const Core::Uint8> Data, const std::filesystem::path& Path, Asset::EMeshImportTextureShape& Shape, Core::FString& Error) const;

        /**
         * @brief FBX 파일 텍스처의 이미지를 내장 바이트에서 찾고 없으면 원본 디렉터리 기준 외부 파일에서 찾습니다.
         * @param[in] Texture 이미지를 찾을 FBX 파일 텍스처입니다.
         * @param[in] SourceDirectory 외부 텍스처 파일을 찾을 원본 FBX 파일의 디렉터리입니다.
         * @param[out] Result 찾은 내장 또는 외부 텍스처 참조를 기록할 대상이며 형태는 판별하지 않습니다.
         * @param[out] Error 이미지를 찾지 못하면 원인을 기록할 대상입니다.
         * @return 이미지를 찾으면 true입니다.
         */
        bool LocateTexture(const fbxsdk::FbxFileTexture& Texture, const std::filesystem::path& SourceDirectory, Asset::FMeshImportTexture& Result, Core::FString& Error) const;
        /**
         * @brief 텍스처 참조의 내장 바이트 또는 외부 파일의 헤더로 판별한 형태를 기록합니다.
         * @param[in,out] Texture 판별할 텍스처 참조이며 성공하면 형태가 기록됩니다.
         * @param[out] Error 읽거나 형식을 인식할 수 없으면 원인을 기록할 대상입니다.
         * @return 판별하면 true입니다.
         */
        bool InspectTexture(Asset::FMeshImportTexture& Texture, Core::FString& Error) const;
        /**
         * @brief FBX 파일 텍스처에 연결된 FbxVideo 중 이미지 시퀀스로 지정된 것이 있는지 확인합니다.
         * @param[in] Texture 확인할 FBX 파일 텍스처입니다.
         * @return 이미지 시퀀스이면 true입니다.
         */
        bool IsImageSequence(const fbxsdk::FbxFileTexture& Texture) const;
        /**
         * @brief FBX 파일 텍스처 하나의 이미지를 찾아 형태를 판별하고 이미지 시퀀스이면 형태를 ImageSequence로 덮어씁니다.
         * @param[in] Texture 판별할 FBX 파일 텍스처입니다.
         * @param[in] SourceDirectory 외부 텍스처 파일을 찾을 원본 FBX 파일의 디렉터리입니다.
         * @param[out] Result 판별한 임포트 텍스처를 기록할 대상입니다.
         * @param[out] Error 이미지를 찾거나 형식을 인식할 수 없으면 원인을 기록할 대상입니다.
         * @return 판별하면 true입니다.
         */
        bool ResolveTexture(const fbxsdk::FbxFileTexture& Texture, const std::filesystem::path& SourceDirectory, Asset::FMeshImportTexture& Result, Core::FString& Error) const;

        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 내장 파일 콜백으로 수집한 텍스처 경로 키별 원본 바이트이며 ResolveTextures 이후 비웁니다.
         */
        FEmbeddedData mEmbeddedData{};
        ReflectProperty(mEmbeddedData, "EmbeddedData", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief ResolveTextures가 형태를 판별한 파일 텍스처별 임포트 텍스처입니다.
         */
        FResolvedTextures mTextures{};
        ReflectProperty(mTextures, "Textures", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief ResolveTextures가 판별하지 못한 파일 텍스처별 오류 메시지입니다.
         */
        FTextureErrors mTextureErrors{};
        ReflectProperty(mTextureErrors, "TextureErrors", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
