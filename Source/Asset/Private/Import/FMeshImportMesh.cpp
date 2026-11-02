#include "Asset/Import/FMeshImportMesh.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace Asset {

    Asset::FMeshImportMesh::FMeshImportMesh(Core::FString Name)
		: mName{std::move(Name)} {
    }

    const Core::FString& Asset::FMeshImportMesh::GetName() const {
        return mName;
    }

    Asset::FMeshData& Asset::FMeshImportMesh::GetMeshData() {
        return mMeshData;
    }

    const Asset::FMeshData& Asset::FMeshImportMesh::GetMeshData() const {
        return mMeshData;
    }

    const Core::TArray<Core::Uint32>& Asset::FMeshImportMesh::GetMaterialIndices() const {
        return mMaterialIndices;
    }

    Core::Uint32 Asset::FMeshImportMesh::FindOrAddMaterialSlot(Core::Uint32 MaterialIndex, const Core::FString& Name) {
        const auto Existing{std::find(mMaterialIndices.begin(), mMaterialIndices.end(), MaterialIndex)};

        if (Existing != mMaterialIndices.end()) {
            return static_cast<Core::Uint32>(Existing - mMaterialIndices.begin());
        }

        mMaterialIndices.push_back(MaterialIndex);
        mMeshData.GetMaterialSlots().emplace_back(Name, Core::FString{});
        return static_cast<Core::Uint32>(mMaterialIndices.size() - 1);
    }

    const Core::TArray<Asset::FMeshImportSkinWeight>& Asset::FMeshImportMesh::GetSkinWeights() const {
        return mSkinWeights;
    }

    void Asset::FMeshImportMesh::AddSkinWeights(std::size_t FirstVertex, Core::TArray<Asset::FMeshImportSkinWeight> Weights) {
        // 메시를 병합하면 스킨 없는 메시 뒤에 스킨 메시가 올 수 있으므로, 그 사이 정점 몫을 영향 없는 가중치로 채웁니다.
        mSkinWeights.resize(std::max(mSkinWeights.size(), FirstVertex));
        mSkinWeights.insert(mSkinWeights.end(), std::make_move_iterator(Weights.begin()), std::make_move_iterator(Weights.end()));
    }

    void Asset::FMeshImportMesh::FinalizeSkinWeights() {
        // 마지막 스킨 메시 뒤에 붙은 스킨 없는 메시의 정점 몫도 영향 없는 가중치로 채워 정점 수와 길이를 맞춥니다.
        if (!mSkinWeights.empty()) {
            mSkinWeights.resize(mMeshData.GetPositions().size());
        }
    }

}
