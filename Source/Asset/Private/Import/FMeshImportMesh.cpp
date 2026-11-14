/**
 * @file FMeshImportMesh.cpp
 * @brief 임포트 결과의 메시 하나와 그 재질 슬롯이 가리키는 결과 재질 인덱스입니다.
 * @ingroup Asset
 */

#include "Core/Types.h"
#include "Asset/Import/FMeshImportMesh.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace Asset {

    /**
     * @brief 이름을 지정해 빈 메시를 생성합니다.
     */
    Asset::FMeshImportMesh::FMeshImportMesh(Core::FString Name)
		: mName{std::move(Name)} {
    }

    /**
     * @brief 메시 이름을 조회합니다.
     */
    const Core::FString& Asset::FMeshImportMesh::GetName() const {
        return mName;
    }

    /**
     * @brief 메시의 정점·인덱스·서브메시·재질 슬롯을 조회합니다.
     */
    Asset::FMeshData& Asset::FMeshImportMesh::GetMeshData() {
        return mMeshData;
    }

    /**
     * @brief 메시의 정점·인덱스·서브메시·재질 슬롯을 조회합니다.
     */
    const Asset::FMeshData& Asset::FMeshImportMesh::GetMeshData() const {
        return mMeshData;
    }

    /**
     * @brief 재질 슬롯별 결과 재질 인덱스를 조회합니다.
     */
    const Core::TArray<Core::Index_t>& Asset::FMeshImportMesh::GetMaterialIndices() const {
        return mMaterialIndices;
    }

    /**
     * @brief 결과 재질을 가리키는 재질 슬롯을 찾고 없으면 새로 추가합니다.
     */
    Core::Index_t Asset::FMeshImportMesh::FindOrAddMaterialSlot(Core::Index_t MaterialIndex, const Core::FString& Name) {
        const auto Existing{std::find(mMaterialIndices.begin(), mMaterialIndices.end(), MaterialIndex)};

        if (Existing != mMaterialIndices.end()) {
            return static_cast<Core::Index_t>(Existing - mMaterialIndices.begin());
        }

        mMaterialIndices.push_back(MaterialIndex);
        mMeshData.GetMaterialSlots().emplace_back(Name, Core::FString{});

        return static_cast<Core::Index_t>(mMaterialIndices.size() - 1);
    }

    /**
     * @brief 정점별 본 가중치를 조회합니다.
     */
    const Core::TArray<Asset::FMeshImportSkinWeight>& Asset::FMeshImportMesh::GetSkinWeights() const {
        return mSkinWeights;
    }

    /**
     * @brief 정점 FirstVertex부터의 본 가중치를 추가합니다. 앞선 스킨 없는 정점 몫은 영향 없는 가중치로 채웁니다.
     */
    void Asset::FMeshImportMesh::AddSkinWeights(Core::Index_t FirstVertex, Core::TArray<Asset::FMeshImportSkinWeight> Weights) {
        // 메시를 병합하면 스킨 없는 메시 뒤에 스킨 메시가 올 수 있으므로, 그 사이 정점 몫을 영향 없는 가중치로 채웁니다.
        mSkinWeights.resize(std::max(mSkinWeights.size(), static_cast<std::size_t>(FirstVertex)));
        mSkinWeights.insert(mSkinWeights.end(), std::make_move_iterator(Weights.begin()), std::make_move_iterator(Weights.end()));
    }

    /**
     * @brief 스킨 정점이 하나라도 있으면 가중치 목록 길이를 정점 수와 맞춥니다. 뒤쪽 스킨 없는 정점은 영향 없는 가중치입니다.
     */
    void Asset::FMeshImportMesh::FinalizeSkinWeights() {
        // 마지막 스킨 메시 뒤에 붙은 스킨 없는 메시의 정점 몫도 영향 없는 가중치로 채워 정점 수와 길이를 맞춥니다.
        if (!mSkinWeights.empty()) {
            mSkinWeights.resize(mMeshData.GetPositions().size());
        }
    }

}
