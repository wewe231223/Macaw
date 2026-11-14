/**
 * @file FMeshImportSkinWeight.h
 * @brief 정점 하나에 영향을 주는 본 인덱스와 가중치입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <array>
#include <utility>

namespace Asset {

    /**
     * @brief 정점 하나에 영향을 주는 본 인덱스와 가중치입니다.
     * @details 가중치가 큰 순서로 최대 MaxInfluences개를 보관하고 합이 1이 되도록 정규화합니다.
     * 쓰지 않는 칸은 본 인덱스 0, 가중치 0이며, 영향이 하나도 없는 정점은 모든 가중치가 0입니다.
     * @ingroup Asset
     */
    class FMeshImportSkinWeight {
    public:
        /**
         * @brief 정점 하나가 가질 수 있는 최대 본 영향 수입니다.
         */
        static constexpr std::size_t MaxInfluences{4};

        /**
         * @brief 영향이 없는 정점의 가중치를 생성합니다.
         */
        FMeshImportSkinWeight() = default;

        /**
         * @brief 정점에 영향을 주는 본 인덱스와 대응 가중치를 저장합니다.
         * @param[in] BoneIndices 영향 슬롯별 본 인덱스 배열입니다.
         * @param[in] Weights 본 영향 슬롯별 스킨 가중치입니다.
         */
        FMeshImportSkinWeight(const std::array<Core::Uint32, MaxInfluences>& BoneIndices, const std::array<float, MaxInfluences>& Weights);

    public:
        /**
         * @brief 본 인덱스와 가중치 목록에서 정점 가중치를 만듭니다.
         * @details 같은 본의 가중치는 합치고, 0 이하이거나 유한하지 않은 가중치는 버린 뒤 큰 순서로 MaxInfluences개를 골라 정규화합니다.
         * @param[in] Influences 결과 본 인덱스와 가중치의 쌍 목록입니다.
         * @return 정리한 정점 가중치이며 남은 영향이 없으면 모든 가중치가 0입니다.
         */
        static Asset::FMeshImportSkinWeight FromInfluences(Core::TArray<std::pair<Core::Index_t, float>> Influences);

        /**
         * @brief 영향을 주는 본 인덱스를 조회합니다.
         * @return 결과 본 목록 기준 인덱스이며 쓰지 않는 칸은 0입니다.
         */
        const std::array<Core::Uint32, MaxInfluences>& GetBoneIndices() const;

        /**
         * @brief 본별 가중치를 조회합니다.
         * @return 합이 1인 가중치이며 쓰지 않는 칸과 영향이 없는 정점은 0입니다.
         */
        const std::array<float, MaxInfluences>& GetWeights() const;

    private:
        /**
         * @brief 결과 본 목록 기준 본 인덱스입니다.
         */
        std::array<Core::Uint32, 4> mBoneIndices{};

        /**
         * @brief 본별 가중치입니다.
         */
        std::array<float, 4> mWeights{};
    };

}
