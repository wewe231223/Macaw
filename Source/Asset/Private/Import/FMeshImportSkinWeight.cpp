/**
 * @file FMeshImportSkinWeight.cpp
 * @brief 정점 하나에 영향을 주는 본 인덱스와 가중치입니다.
 * @ingroup Asset
 */

#include "Core/Types.h"
#include "Asset/Import/FMeshImportSkinWeight.h"

#include <algorithm>
#include <cmath>

namespace Asset {

    /**
     * @brief 정점에 영향을 주는 본 인덱스와 대응 가중치를 저장합니다.
     */
    Asset::FMeshImportSkinWeight::FMeshImportSkinWeight(const std::array<Core::Uint32, MaxInfluences>& BoneIndices, const std::array<float, MaxInfluences>& Weights)
		: mBoneIndices{BoneIndices},
		  mWeights{Weights} {
    }

    /**
     * @brief 본 인덱스와 가중치 목록에서 정점 가중치를 만듭니다.
     */
    Asset::FMeshImportSkinWeight Asset::FMeshImportSkinWeight::FromInfluences(Core::TArray<std::pair<Core::Index_t, float>> Influences) {
        // 여러 클러스터가 같은 본을 가리킬 수 있으므로 본 인덱스로 정렬해 같은 본의 가중치를 합칩니다.
        std::sort(Influences.begin(), Influences.end());

        Core::TArray<std::pair<Core::Index_t, float>> Merged{};

        for (const auto& [Bone, Weight] : Influences) {
            if (!std::isfinite(Weight) || Weight <= 0.0f) {
                continue;
            }

            if (!Merged.empty() && Merged.back().first == Bone) {
                Merged.back().second += Weight;
            } else {
                Merged.emplace_back(Bone, Weight);
            }
        }

        // 가중치가 큰 영향부터 MaxInfluences개만 남깁니다. 같은 가중치는 본 인덱스가 작은 쪽을 먼저 두어 결과를 결정적으로 만듭니다.
        std::stable_sort(Merged.begin(), Merged.end(), [](const auto& Left, const auto& Right) {
            return Left.second > Right.second;
        });

        Asset::FMeshImportSkinWeight Result{};
        const std::size_t Count{std::min(Merged.size(), MaxInfluences)};
        float Total{};

        for (Core::Index_t Index{}; Index < Count; ++Index) {
            Total += Merged[Index].second;
        }

        if (Total <= 0.0f || !std::isfinite(Total)) {
            return Result;
        }

        // 버린 영향만큼 줄어든 합을 1로 다시 맞춥니다.
        for (Core::Index_t Index{}; Index < Count; ++Index) {
            Result.mBoneIndices[Index] = static_cast<Core::Uint32>(Merged[Index].first);
            Result.mWeights[Index] = Merged[Index].second / Total;
        }

        return Result;
    }

    /**
     * @brief 영향을 주는 본 인덱스를 조회합니다.
     */
    const std::array<Core::Uint32, Asset::FMeshImportSkinWeight::MaxInfluences>& Asset::FMeshImportSkinWeight::GetBoneIndices() const {
        return mBoneIndices;
    }

    /**
     * @brief 본별 가중치를 조회합니다.
     */
    const std::array<float, Asset::FMeshImportSkinWeight::MaxInfluences>& Asset::FMeshImportSkinWeight::GetWeights() const {
        return mWeights;
    }

}
