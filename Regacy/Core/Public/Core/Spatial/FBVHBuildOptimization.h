/**
 * @file FBVHBuildOptimization.h
 * @brief 이진 구축 트리의 경계 병합, 8분기 비용 추정 및 작은 하위 트리 재구성을 정의한다.
 */
#pragma once
#include "Core/Common.h"
#include "Math/FMath.h"
#include <array>
#include <bit>
#include <cmath>
#include <limits>

namespace BVH8 {
    /**
     * @brief 구축 비용에 사용할 경계 표면적의 비례값을 계산한다.
     * @param Box 비용을 계산할 축 정렬 경계 상자.
     * @return 반크기 성분으로 계산한 xy + yz + zx. 실제 표면적의 1/8에 해당한다.
     */
    inline double BuildArea(const DirectX::BoundingBox& Box) {
        return double(Box.Extents.x) * Box.Extents.y + double(Box.Extents.y) * Box.Extents.z + double(Box.Extents.z) * Box.Extents.x;
    }

    /**
     * @brief 두 경계 상자를 포함하는 구축용 경계를 구성한다.
     * @param A 첫 번째 경계 상자.
     * @param B 두 번째 경계 상자.
     * @return 두 상자를 포함하는 축 정렬 경계 상자.
     * @details 반크기를 nextafter()로 양의 무한대 방향으로 한 단계 확장하여 반올림으로 경계가 좁아지는 것을 방지한다.
     */
    inline DirectX::BoundingBox MergeBuildBounds(const DirectX::BoundingBox& A, const DirectX::BoundingBox& B) {
        const float AC[]{A.Center.x, A.Center.y, A.Center.z}, AE[]{A.Extents.x, A.Extents.y, A.Extents.z}, BC[]{B.Center.x, B.Center.y, B.Center.z}, BE[]{B.Extents.x, B.Extents.y, B.Extents.z};

        float Center[3], Extents[3];

        for (Uint32 Axis = 0; Axis < 3; ++Axis) {
            const float Min = (std::min)(AC[Axis] - AE[Axis], BC[Axis] - BE[Axis]), Max = (std::max)(AC[Axis] + AE[Axis], BC[Axis] + BE[Axis]);

            Center[Axis] = Min * 0.5f + Max * 0.5f;
            Extents[Axis] = std::nextafter((std::max)(Center[Axis] - Min, Max - Center[Axis]), std::numeric_limits<float>::infinity());
        }

        return {{Center[0], Center[1], Center[2]}, {Extents[0], Extents[1], Extents[2]}};
    }

    /**
     * @brief 이진 트리를 최대 8분기로 확장할 때의 경계 면적 기반 최소 비용을 추정한다.
     * @tparam TNode BoundingBox, mLeft 및 mRight를 제공하는 노드 타입.
     * @param Nodes 인덱스 0이 루트이며 mLeft가 Uint32 최댓값이면 리프인 유효한 이진 트리.
     * @return 추정한 내부 경계 순회 비용. 빈 입력이면 0.
     * @details 리프 비용은 0으로 두고 각 하위 트리의 1부터 8까지의 분기 구성을 평가한다.
     */
    template <class TNode>
    double EstimateWideBuildCost(const TArray<TNode>& Nodes) {
        const auto Evaluate = [&](auto&& Self, Uint32 Index) -> std::array<double, 9> {
            std::array<double, 9> Cost;

            Cost.fill(std::numeric_limits<double>::infinity());

            const auto& Node = Nodes[Index];

            if (Node.mLeft == std::numeric_limits<Uint32>::max()) {
                Cost[1] = 0;
                return Cost;
            }

            const auto Left = Self(Self, Node.mLeft), Right = Self(Self, Node.mRight);
            double Best = std::numeric_limits<double>::infinity();

            for (Uint32 Count = 2; Count <= 8; ++Count) {
                for (Uint32 Split = 1; Split < Count; ++Split)
                    Cost[Count] = (std::min)(Cost[Count], Left[Split] + Right[Count - Split]);

                Best = (std::min)(Best, Cost[Count]);
            }

            Cost[1] = BuildArea(Node.BoundingBox) + Best;

            return Cost;
        };

        return Nodes.empty() ? 0.0 : Evaluate(Evaluate, 0)[1];
    }

    /**
     * @brief 작은 하위 트리를 재구성하여 전체 8분기 구축 비용을 줄인다.
     * @tparam TNode 복사 가능하고 BoundingBox, mLeft 및 mRight를 제공하는 노드 타입.
     * @param[in,out] Nodes 인덱스 0이 루트인 유효한 이진 트리. 채택한 후보가 있을 때만 교체한다.
     * @return 후보의 추정 비용이 원본의 99% 미만이라 채택하면 true, 그 외에는 false.
     * @details 최대 일곱 개의 경계로 하위 트리를 확장하고 부분집합 분할 비용으로 재구성한다. 노드가 15개 미만이거나 원본 비용이 유한한 양수가 아니면 최적화하지 않는다.
     */
    template <class TNode>
    bool OptimizeBuildTreelets(TArray<TNode>& Nodes) {
        if (Nodes.size() < 15)
            return false;

        const double OriginalCost = EstimateWideBuildCost(Nodes);

        if (!std::isfinite(OriginalCost) || OriginalCost <= 0)
            return false;

        TArray<TNode> Candidate = Nodes;
        TArray<double> Costs(Nodes.size());

        const auto Optimize = [&](auto&& Self, Uint32 Root) -> void {
            auto& Node = Candidate[Root];

            if (Node.mLeft == std::numeric_limits<Uint32>::max()) {
                Costs[Root] = 0;
                return;
            }

            Self(Self, Node.mLeft);
            Self(Self, Node.mRight);
            Node.BoundingBox = MergeBuildBounds(Candidate[Node.mLeft].BoundingBox, Candidate[Node.mRight].BoundingBox);
            Costs[Root] = BuildArea(Node.BoundingBox) + Costs[Node.mLeft] + Costs[Node.mRight];

            Uint32 Frontier[7]{Root}, Slots[6]{};
            Uint32 Count = 1, SlotCount = 0;

            while (Count < 7) {
                Uint32 Best = Count;
                double Largest = -1;

                for (Uint32 i = 0; i < Count; ++i)
                    if (Candidate[Frontier[i]].mLeft != std::numeric_limits<Uint32>::max() && BuildArea(Candidate[Frontier[i]].BoundingBox) > Largest) {
                        Best = i;
                        Largest = BuildArea(Candidate[Frontier[i]].BoundingBox);
                    }
                if (Best == Count)
                    break;

                const Uint32 Index = Frontier[Best];

                Slots[SlotCount++] = Index;
                Frontier[Best] = Candidate[Index].mLeft;
                Frontier[Count++] = Candidate[Index].mRight;
            }

            if (Count < 3)
                return;

            DirectX::BoundingBox Bounds[128];
            double Best[128]{};
            Uint32 Split[128]{};
            const Uint32 Full = (1u << Count) - 1;

            for (Uint32 Mask = 1; Mask <= Full; ++Mask) {
                const Uint32 Bit = Mask & (~Mask + 1), Rest = Mask ^ Bit, Index = Frontier[std::countr_zero(Bit)];

                if (Rest == 0) {
                    Bounds[Mask] = Candidate[Index].BoundingBox;
                    Best[Mask] = Costs[Index];
                    continue;
                }

                Bounds[Mask] = MergeBuildBounds(Bounds[Bit], Bounds[Rest]);
                Best[Mask] = std::numeric_limits<double>::infinity();

                const double Area = BuildArea(Bounds[Mask]);

                for (Uint32 Left = (Mask - 1) & Mask; Left != 0; Left = (Left - 1) & Mask) {
                    if ((Left & Bit) == 0)
                        continue;

                    const double Cost = Best[Left] + Best[Mask ^ Left] + Area;

                    if (Cost < Best[Mask]) {
                        Best[Mask] = Cost;
                        Split[Mask] = Left;
                    }
                }
            }

            if (Best[Full] >= Costs[Root] * (1.0 - 1e-6))
                return;

            Uint32 Next = 0;

            const auto Rebuild = [&](auto&& Recur, Uint32 Mask) -> Uint32 {
                if (std::has_single_bit(Mask))
                    return Frontier[std::countr_zero(Mask)];

                const Uint32 Index = Slots[Next++], Left = Recur(Recur, Split[Mask]), Right = Recur(Recur, Mask ^ Split[Mask]);

                Candidate[Index].mLeft = Left;
                Candidate[Index].mRight = Right;
                Candidate[Index].BoundingBox = MergeBuildBounds(Candidate[Left].BoundingBox, Candidate[Right].BoundingBox);
                Costs[Index] = BuildArea(Candidate[Index].BoundingBox) + Costs[Left] + Costs[Right];

                return Index;
            };

            Rebuild(Rebuild, Full);
        };

        Optimize(Optimize, 0);

        if (!(EstimateWideBuildCost(Candidate) < OriginalCost * 0.99))
            return false;

        Nodes.swap(Candidate);

        return true;
    }
}
