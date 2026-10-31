/**
 * @file FAssimpMeshImporter.h
 * @brief Assimp로 원본 메시를 삼각형·좌수 좌표계로 변환하고 재질과 텍스처 참조를 수집합니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Import/IMeshImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>

namespace Asset {

    /**
     * @brief Assimp로 원본 메시를 삼각형·좌수 좌표계로 변환하고 재질과 텍스처 참조를 수집합니다.
     */
    class FAssimpMeshImporter final : public Asset::IMeshImporter {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FAssimpMeshImporter, Asset::IMeshImporter);

    public:
        /**
         * @brief 가져올 원본 파일 경로를 보관하도록 Assimp 임포터를 초기화합니다.
         * @param[in] Source 가져올 원본 메시 파일 경로입니다.
         */
        explicit FAssimpMeshImporter(std::filesystem::path Source);
        /**
         * @brief Assimp 임포터와 읽어 들인 장면을 해제합니다.
         */
        ~FAssimpMeshImporter() override;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FAssimpMeshImporter);

    private:
        /**
         * @brief Assimp로 원본 메시 파일을 읽어 정점·삼각형·재질 슬롯과 텍스처 참조를 수집합니다.
         * @return 유효한 임포트 결과를 구성하면 true이며 실패하면 Error에 원인을 기록하고 false입니다.
         */
        bool ImportSource() override;

        /**
         * @brief Assimp 재질에서 우선 종류의 텍스처를 찾고 없으면 대체 종류의 텍스처 참조를 읽습니다.
         * @param[in] Material 텍스처 참조를 읽을 Assimp 재질입니다.
         * @param[in] PrimaryType 먼저 찾을 텍스처 종류입니다.
         * @param[in] FallbackType 우선 종류가 없을 때 찾을 텍스처 종류입니다.
         * @return 텍스처가 없으면 None, 장면 내장 텍스처이면 Embedded, 그 외에는 원본 디렉터리 기준 외부 파일 참조입니다.
         */
        Asset::FMeshImportTexture ReadTexture(const aiMaterial& Material, aiTextureType PrimaryType, aiTextureType FallbackType) const;
        /**
         * @brief 장면의 모든 Assimp 메시를 하나의 메시 데이터로 합치고 재질별 섹션을 구성합니다.
         * @return 성공하면 true이며 빈 메시이거나 버퍼 한계를 넘으면 Error에 원인을 기록하고 false입니다.
         */
        bool ReadMesh();
        /**
         * @brief 장면의 Assimp 재질마다 기본 색상·불투명도·양면 여부와 기본 색상·노멀 텍스처를 읽어 재질 목록에 추가합니다.
         */
        bool ReadMaterials();

        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 원본 파일을 읽고 읽어 들인 장면의 수명을 소유하는 Assimp 임포터입니다.
         */
        Assimp::Importer mImporter{};
        ReflectProperty(mImporter, "Importer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief mImporter가 소유하는 읽어 들인 장면의 비소유 주소입니다.
         */
        const aiScene* mScene{};
        ReflectProperty(mScene, "Scene", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
