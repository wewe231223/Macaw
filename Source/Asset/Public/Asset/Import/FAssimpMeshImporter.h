/**
 * @file FAssimpMeshImporter.h
 * @brief Assimp로 원본 메시를 삼각형·좌수 좌표계로 변환하고 재질과 텍스처 참조를 수집합니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "Asset/Import/IMeshImporter.h"
#include "Core/Math/FMatrix.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>

struct aiMesh;
struct aiNode;

namespace Asset {

    class FMeshImportTransform;

    /**
     * @brief Assimp로 원본 메시를 삼각형·좌수 좌표계로 변환하고 재질과 텍스처 참조를 수집합니다.
     * @details 지원하는 원본(OBJ, glTF)은 Y-Up 오른손 좌표계이므로 FBX 임포터와 같은 축 옵션 결과가 나오도록 직접 변환합니다.
     */
    class FAssimpMeshImporter final : public Asset::IMeshImporter {
    public:
        /**
         * @brief 가져올 원본 파일 경로와 임포트 옵션을 보관하도록 Assimp 임포터를 초기화합니다.
         * @param[in] Source 가져올 원본 메시 파일 경로입니다.
         * @param[in] Option 좌표축·메시 구성·노멀 처리 방식을 정하는 임포트 옵션입니다.
         */
        FAssimpMeshImporter(std::filesystem::path Source, const Asset::FAssetImportOption& Option);

        /**
         * @brief Assimp 임포터와 읽어 들인 장면을 해제합니다.
         */
        ~FAssimpMeshImporter() override;

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
         * @brief 노드와 모든 자식 노드를 깊이 우선으로 순회하며 노드의 메시를 읽습니다.
         * @details 메시 병합 옵션을 켜면 결과의 첫 메시에, 끄면 메시를 가진 노드마다 새 결과 메시에 추가합니다.
         * @param[in] Node 순회를 시작할 Assimp 노드입니다.
         * @param[in] ParentTransform 부모 노드까지의 원본 공간 전역 변환입니다.
         * @return 모든 메시를 읽으면 true이며 하나라도 실패하면 Error에 원인을 기록하고 false입니다.
         */
        bool ReadNode(const aiNode& Node, const Core::FMatrix& ParentTransform);

        /**
         * @brief Assimp 메시의 정점과 삼각형을 엔진 공간으로 변환해 대상 메시에 추가하고 서브메시를 구성합니다.
         * @param[in] Source 읽을 Assimp 메시입니다.
         * @param[in] Transform 메시 공간에서 엔진 공간으로 가는 변환입니다.
         * @param[in,out] Target 정점과 서브메시를 이어 붙일 결과 메시입니다.
         * @return 성공하면 true이며 빈 메시이거나 버퍼 한계를 넘으면 Error에 원인을 기록하고 false입니다.
         */
        bool ReadMesh(const aiMesh& Source, const Asset::FMeshImportTransform& Transform, Asset::FMeshImportMesh& Target);

        /**
         * @brief 메시 노드를 순회하며 메시 본의 오프셋 행렬로 본 이름별 바인드 시점 전역 변환을 모읍니다.
         * @param[in] Node 순회를 시작할 Assimp 노드입니다.
         * @param[in] ParentTransform 부모 노드까지의 원본 공간 전역 변환입니다.
         * @return 모든 오프셋 행렬을 되돌릴 수 있으면 true이며 실패하면 Error에 원인을 기록하고 false입니다.
         */
        bool CollectBindPoses(const aiNode& Node, const Core::FMatrix& ParentTransform);

        /**
         * @brief 메시가 참조하는 본 이름과 같은 노드를 깊이 우선으로 순회하며 결과 본 목록에 추가합니다.
         * @param[in] Node 순회를 시작할 Assimp 노드입니다.
         * @param[in] ParentIndex 가장 가까운 조상 본의 결과 인덱스이며 없으면 -1입니다.
         * @param[in] ParentBoneGlobal 가장 가까운 조상 본의 엔진 공간 전역 변환이며 없으면 항등 행렬입니다.
         * @return 모든 본을 읽으면 true이며 바인드 포즈를 변환할 수 없으면 false입니다.
         */
        bool ReadSkeleton(const aiNode& Node, Core::Index_t ParentIndex, const Core::FMatrix& ParentBoneGlobal);

        /**
         * @brief 장면의 Assimp 재질마다 기본 색상·불투명도·양면 여부와 기본 색상·노멀 텍스처를 읽어 재질 목록에 추가합니다.
         * @return 성공하면 true이며 조명 속성이 유효하지 않으면 Error에 원인을 기록하고 false입니다.
         */
        bool ReadMaterials();

    private:
        /**
         * @brief 원본 파일을 읽고 읽어 들인 장면의 수명을 소유하는 Assimp 임포터입니다.
         */
        Assimp::Importer mImporter{};

        /**
         * @brief mImporter가 소유하는 읽어 들인 장면의 비소유 주소입니다.
         */
        const aiScene* mScene{};

        /**
         * @brief 원본 전역 공간에서 엔진 공간으로 가는 행렬입니다. 축 변환과 임포트 이동·회전·스케일을 포함합니다.
         */
        Core::FMatrix mBasis{};

        /**
         * @brief 메시 본 이름별 바인드 시점 전역 변환이며 원본 전역 공간 기준입니다.
         */
        Core::TMap<Core::FString, Core::FMatrix> mBindPoses{};

        /**
         * @brief 본으로 읽은 노드 이름별 결과 본 인덱스입니다.
         */
        Core::TMap<Core::FString, Core::Index_t> mBoneIndices{};
    };

}
