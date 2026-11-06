/**
 * @file FMeshImportMesh.h
 * @brief 임포트 결과의 메시 하나와 그 재질 슬롯이 가리키는 결과 재질 인덱스입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "Asset/FMeshData.h"
#include "Asset/Import/FMeshImportSkinWeight.h"

namespace Asset {

    /**
     * @brief 임포트 결과의 메시 하나와 그 재질 슬롯이 가리키는 결과 재질 인덱스입니다.
     * @details 메시 데이터의 재질 슬롯 i는 결과 재질 목록의 GetMaterialIndices()[i]번 재질에 대응합니다.
     * @ingroup Asset
     */
    class FMeshImportMesh {
    public:
        /**
         * @brief 빈 메시를 생성합니다.
         */
        FMeshImportMesh() = default;

        /**
         * @brief 이름을 지정해 빈 메시를 생성합니다.
         * @param[in] Name 원본 노드 또는 파일에서 가져온 메시 이름입니다.
         */
        explicit FMeshImportMesh(Core::FString Name);

    public:
        /**
         * @brief 메시 이름을 조회합니다.
         * @return 원본에서 가져온 메시 이름의 읽기 전용 참조입니다.
         */
        const Core::FString& GetName() const;

        /**
         * @brief 메시의 정점·인덱스·서브메시·재질 슬롯을 조회합니다.
         * @return 메시가 소유하는 메시 데이터의 참조입니다.
         */
        Asset::FMeshData& GetMeshData();

        /**
         * @brief 메시의 정점·인덱스·서브메시·재질 슬롯을 조회합니다.
         * @return 메시가 소유하는 메시 데이터의 참조입니다.
         */
        const Asset::FMeshData& GetMeshData() const;

        /**
         * @brief 재질 슬롯별 결과 재질 인덱스를 조회합니다.
         * @return 재질 슬롯과 같은 길이의 인덱스 목록입니다.
         */
        const Core::TArray<Core::Uint32>& GetMaterialIndices() const;

        /**
         * @brief 결과 재질을 가리키는 재질 슬롯을 찾고 없으면 새로 추가합니다.
         * @param[in] MaterialIndex 결과 재질 목록에서의 인덱스입니다.
         * @param[in] Name 새 슬롯에 기록할 재질 이름입니다.
         * @return 이 메시에서의 재질 슬롯 인덱스입니다.
         */
        Core::Uint32 FindOrAddMaterialSlot(Core::Uint32 MaterialIndex, const Core::FString& Name);

        /**
         * @brief 정점별 본 가중치를 조회합니다.
         * @return 스킨이 없는 메시이면 빈 목록이고, 있으면 정점 수와 같은 길이의 목록입니다.
         */
        const Core::TArray<Asset::FMeshImportSkinWeight>& GetSkinWeights() const;

        /**
         * @brief 정점 FirstVertex부터의 본 가중치를 추가합니다. 앞선 스킨 없는 정점 몫은 영향 없는 가중치로 채웁니다.
         * @param[in] FirstVertex Weights[0]이 대응하는 정점 인덱스이며 지금까지의 가중치 수 이상이어야 합니다.
         * @param[in] Weights 이어서 추가할 정점별 가중치입니다.
         */
        void AddSkinWeights(std::size_t FirstVertex, Core::TArray<Asset::FMeshImportSkinWeight> Weights);

        /**
         * @brief 스킨 정점이 하나라도 있으면 가중치 목록 길이를 정점 수와 맞춥니다. 뒤쪽 스킨 없는 정점은 영향 없는 가중치입니다.
         */
        void FinalizeSkinWeights();

    private:
        /**
         * @brief 원본 노드 또는 파일에서 가져온 메시 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 엔진 공간으로 변환한 정점·인덱스·서브메시·재질 슬롯입니다.
         */
        Asset::FMeshData mMeshData{};

        /**
         * @brief 재질 슬롯별 결과 재질 인덱스입니다.
         */
        Core::TArray<Core::Uint32> mMaterialIndices{};

        /**
         * @brief 정점별 본 가중치이며 스킨이 없는 메시이면 비어 있습니다.
         */
        Core::TArray<Asset::FMeshImportSkinWeight> mSkinWeights{};
    };

}
