/**
 * @file FBVH8.h
 * @brief 8분기 BVH의 참조 인코딩, 노드 저장 형식, 이진 트리 변환 및 광선 순회를 정의한다.
 */
/**
 * @var Uint32 BVH8::FLeaf::Count
 * @brief 리프 또는 삼각형 패킷에서 처리할 1부터 8까지의 유효 요소 수입니다.
 */
/**
 * @var float BVH8::FNode::MaxX[8]
 * @brief 각 자식 경계의 X축 최댓값을 저장한 여덟 레인 배열입니다.
 */
/**
 * @var float BVH8::FNode::MaxY[8]
 * @brief 각 자식 경계의 Y축 최댓값을 저장한 여덟 레인 배열입니다.
 */
/**
 * @var float BVH8::FNode::MaxZ[8]
 * @brief 각 자식 경계의 Z축 최댓값을 저장한 여덟 레인 배열입니다.
 */
#pragma once
#include "Core/Base/ErrorHandler.h"
#include "Core/Common.h"
#include "Math/FMath.h"
#include <array>
#include <bit>
#include <limits>

/**
 * @brief 8분기 경계 볼륨 계층의 구축, SIMD 순회 및 삼각형 패킷 교차 검사를 지원한다.
 */
namespace BVH8 {
    /**
     * @brief 참조의 최상위 비트에 사용하는 리프 표시 마스크.
     */
    constexpr Uint32 LeafBit = 0x80000000u;
    /**
     * @brief 참조에서 개수보다 1 작은 값을 저장하는 3비트 필드의 시작 위치.
     */
    constexpr Uint32 CountShift = 28;
    /**
     * @brief 참조의 하위 28비트에서 노드 또는 리프 인덱스를 추출하는 마스크.
     */
    constexpr Uint32 IndexMask = (1u << CountShift) - 1;

    /**
     * @brief 인코딩된 참조에서 자식 또는 리프 요소 수를 추출한다.
     * @param Reference MakeReference()로 구성한 참조.
     * @return 개수 필드에 1을 더한 1부터 8까지의 값.
     */
    constexpr Uint32 GetCount(Uint32 Reference) {
        return ((Reference >> CountShift) & 7u) + 1;
    }

    /**
     * @brief 인덱스, 개수 및 리프 여부를 하나의 32비트 참조로 인코딩한다.
     * @param Index IndexMask 이하의 노드 또는 리프 인덱스.
     * @param Count 자식 또는 리프 요소 수. 1부터 8까지 허용한다.
     * @param Leaf true이면 리프 표시 비트를 설정한다.
     * @return 인코딩된 참조.
     * @details 인덱스나 개수가 허용 범위를 벗어나면 치명적 오류를 보고한다.
     */
    Uint32 MakeReference(Uint32 Index, Uint32 Count, bool Leaf = false);

    /**
     * @brief 리프 또는 삼각형 패킷의 인덱스와 묶음 크기를 보관한다.
     */
    struct FLeaf {
        /**
         * @brief Index는 리프 또는 패킷 인덱스이고 Count는 그 리프에서 처리할 1부터 8까지의 요소 수이다.
         */
        Uint32 Index, Count;
    };

    /**
     * @brief 묶음 크기를 가진 리프 정보를 참조로 인코딩한다.
     * @param Leaf 리프 인덱스와 요소 수.
     * @return 리프 비트가 설정된 참조.
     */
    inline Uint32 MakeLeafReference(FLeaf Leaf) {
        return MakeReference(Leaf.Index, Leaf.Count, true);
    }

    /**
     * @brief 요소 하나를 가진 리프 참조를 구성한다.
     * @param Index 리프 인덱스.
     * @return 리프 비트와 개수 1을 인코딩한 참조.
     */
    inline Uint32 MakeLeafReference(Uint32 Index) {
        return MakeReference(Index, 1, true);
    }

    /**
     * @brief 여덟 자식의 축별 경계와 참조를 구조체 배열 대신 성분별 배열로 저장하는 32바이트 정렬 노드. 크기는 224바이트이다.
     */
    struct alignas(32) FNode {
        /**
         * @brief 여덟 자식 경계의 X축 최솟값과 최댓값 배열.
         */
        float MinX[8]{}, MaxX[8]{};
        /**
         * @brief 여덟 자식 경계의 Y축 최솟값과 최댓값 배열.
         */
        float MinY[8]{}, MaxY[8]{};
        /**
         * @brief 여덟 자식 경계의 Z축 최솟값과 최댓값 배열.
         */
        float MinZ[8]{}, MaxZ[8]{};
        /**
         * @brief 각 자식의 인코딩된 내부 노드 또는 리프 참조. 유효한 배열 구간은 부모 참조의 개수로 결정한다.
         */
        Uint32 Children[8]{};
    };

    static_assert(sizeof(FNode) == 224 && alignof(FNode) == 32);

    /**
     * @brief 축별 경계 검사에 사용할 광선 원점, 역방향 성분 및 축 평행 여부를 보관한다.
     */
    struct FRayData {
        /**
         * @brief 광선 원점의 X, Y, Z 성분.
         */
        float Origin[3];
        /**
         * @brief 방향 성분의 역수. 축 평행으로 판정한 성분은 검사 중 별도로 처리하며 이 값에는 1을 저장한다.
         */
        float InverseDirection[3];
        /**
         * @brief 방향 성분의 절댓값이 DirectX의 광선 오차 한계 이하인지 나타내는 축별 상태.
         */
        bool Parallel[3];

        /**
         * @brief 광선에서 축별 순회용 데이터를 계산한다.
         * @param Ray 검사할 원점과 방향을 가진 광선.
         */
        explicit FRayData(const FRay& Ray);
    };

    /**
     * @brief 문맥 포인터, 리프 인덱스 및 최근접 거리 참조를 받아 실제 충돌 여부를 반환하는 리프 콜백. 묶음 크기는 전달하지 않는다.
     */
    using FVisitLeaf = bool (*)(void*, Uint32, float&);
    /**
     * @brief 내부 노드 배열을 순회하며 사용자 리프 콜백을 호출하는 광선 검사 함수 포인터 타입.
     */
    using FRaycaster = bool (*)(const FNode*, Uint32, const FRayData&, float&, void*, FVisitLeaf);
    struct FTrianglePacket;
    /**
     * @brief 삼각형 패킷과 와인딩 방향을 사용하는 광선 검사 함수 포인터 타입.
     */
    using FTrianglePacketRaycaster = bool (*)(const FNode*, Uint32, const FRayData&, const FRay&, float&, const FTrianglePacket*, bool);
    /**
     * @brief CPU와 운영체제의 AVX 지원에 따라 일반 및 삼각형 패킷 순회의 SSE 또는 AVX 구현을 선택한다. 초기 선택은 SSE이다.
     */
    void Initialize();
    /**
     * @brief 선택된 SIMD 구현으로 경계 트리를 순회하고 사용자 리프 콜백을 호출한다.
     * @param Nodes 참조 인덱스로 접근할 내부 노드 배열.
     * @param RootReference 시작할 인코딩된 노드 또는 리프 참조.
     * @param Ray 축별 순회용 광선 데이터.
     * @param[in,out] ClosestDistance 광선 매개변수 상한. 콜백이 더 가까운 값으로 갱신한다.
     * @param Context 리프 콜백에 전달할 사용자 문맥.
     * @param VisitLeaf 실제 리프 교차를 검사할 콜백.
     * @return 리프 콜백 중 하나라도 충돌을 보고하면 true.
     */
    bool Raycast(const FNode* Nodes, Uint32 RootReference, const FRayData& Ray, float& ClosestDistance, void* Context, FVisitLeaf VisitLeaf);
    /**
     * @brief SSE 구현으로 경계 트리를 순회하고 사용자 리프 콜백을 호출한다.
     * @param Nodes 참조 인덱스로 접근할 내부 노드 배열.
     * @param RootReference 시작할 인코딩된 노드 또는 리프 참조.
     * @param Ray 축별 순회용 광선 데이터.
     * @param[in,out] ClosestDistance 광선 매개변수 상한. 콜백이 더 가까운 값으로 갱신한다.
     * @param Context 리프 콜백에 전달할 사용자 문맥.
     * @param VisitLeaf 실제 리프 교차를 검사할 콜백.
     * @return 리프 콜백 중 하나라도 충돌을 보고하면 true.
     */
    bool RaycastSSE(const FNode* Nodes, Uint32 RootReference, const FRayData& Ray, float& ClosestDistance, void* Context, FVisitLeaf VisitLeaf);
    /**
     * @brief AVX 구현으로 경계 트리를 순회하고 사용자 리프 콜백을 호출한다.
     * @param Nodes 참조 인덱스로 접근할 내부 노드 배열.
     * @param RootReference 시작할 인코딩된 노드 또는 리프 참조.
     * @param Ray 축별 순회용 광선 데이터.
     * @param[in,out] ClosestDistance 광선 매개변수 상한. 콜백이 더 가까운 값으로 갱신한다.
     * @param Context 리프 콜백에 전달할 사용자 문맥.
     * @param VisitLeaf 실제 리프 교차를 검사할 콜백.
     * @return 리프 콜백 중 하나라도 충돌을 보고하면 true.
     * @pre CPU와 운영체제가 AVX를 지원해야 한다.
     */
    bool RaycastAVX(const FNode* Nodes, Uint32 RootReference, const FRayData& Ray, float& ClosestDistance, void* Context, FVisitLeaf VisitLeaf);
    /**
     * @brief CPU의 AVX 및 운영체제의 XMM/YMM 상태 저장 지원을 확인한다.
     * @return 필요한 CPU 기능과 운영체제 지원이 모두 있으면 true.
     * @details 처음 검사한 결과를 정적 값으로 보관한다.
     */
    bool SupportsAVX();
    /**
     * @brief 선택된 SIMD 구현으로 삼각형 패킷의 최근접 교차를 검사한다.
     * @param Nodes 참조 인덱스로 접근할 내부 노드 배열.
     * @param RootReference 시작할 인코딩된 참조.
     * @param RayData Ray와 같은 광선에서 준비한 축별 순회 데이터.
     * @param Ray 삼각형 교차를 검사할 광선.
     * @param[in,out] ClosestDistance 광선 매개변수 상한. 충돌 시 가장 가까운 값으로 갱신한다.
     * @param Packets 리프 인덱스로 접근할 삼각형 패킷 배열.
     * @param ReverseWinding true이면 면의 와인딩 판정을 뒤집는다.
     * @return 상한 안에 유효한 삼각형 교차가 있으면 true.
     * @details 리프 참조의 개수만큼 패킷의 유효한 삼각형을 검사하며 뒷면은 제외한다.
     */
    bool RaycastTrianglePackets(const FNode* Nodes, Uint32 RootReference, const FRayData& RayData, const FRay& Ray, float& ClosestDistance, const FTrianglePacket* Packets, bool ReverseWinding = false);
    /**
     * @brief SSE 구현으로 삼각형 패킷의 최근접 교차를 검사한다.
     * @param Nodes 참조 인덱스로 접근할 내부 노드 배열.
     * @param RootReference 시작할 인코딩된 참조.
     * @param RayData Ray와 같은 광선에서 준비한 축별 순회 데이터.
     * @param Ray 삼각형 교차를 검사할 광선.
     * @param[in,out] ClosestDistance 광선 매개변수 상한. 충돌 시 가장 가까운 값으로 갱신한다.
     * @param Packets 리프 인덱스로 접근할 삼각형 패킷 배열.
     * @param ReverseWinding true이면 면의 와인딩 판정을 뒤집는다.
     * @return 상한 안에 유효한 삼각형 교차가 있으면 true.
     * @details 리프 참조의 개수만큼 패킷의 유효한 삼각형을 검사하며 뒷면은 제외한다.
     */
    bool RaycastTrianglePacketsSSE(const FNode* Nodes, Uint32 RootReference, const FRayData& RayData, const FRay& Ray, float& ClosestDistance, const FTrianglePacket* Packets, bool ReverseWinding = false);
    /**
     * @brief AVX 구현으로 삼각형 패킷의 최근접 교차를 검사한다.
     * @param Nodes 참조 인덱스로 접근할 내부 노드 배열.
     * @param RootReference 시작할 인코딩된 참조.
     * @param RayData Ray와 같은 광선에서 준비한 축별 순회 데이터.
     * @param Ray 삼각형 교차를 검사할 광선.
     * @param[in,out] ClosestDistance 광선 매개변수 상한. 충돌 시 가장 가까운 값으로 갱신한다.
     * @param Packets 리프 인덱스로 접근할 삼각형 패킷 배열.
     * @param ReverseWinding true이면 면의 와인딩 판정을 뒤집는다.
     * @return 상한 안에 유효한 삼각형 교차가 있으면 true.
     * @details 리프 참조의 개수만큼 패킷의 유효한 삼각형을 검사하며 뒷면은 제외한다.
     * @pre CPU와 운영체제가 AVX를 지원해야 한다.
     */
    bool RaycastTrianglePacketsAVX(const FNode* Nodes, Uint32 RootReference, const FRayData& RayData, const FRay& Ray, float& ClosestDistance, const FTrianglePacket* Packets, bool ReverseWinding = false);
}

/**
 * @brief 이진 경계 트리를 최대 8분기 노드 배열로 변환하고 최근접 광선 검사를 제공한다.
 * @details 노드 참조의 개수 필드로 유효한 자식만 순회하며 리프 저장소 자체는 외부 콜백 또는 삼각형 패킷 배열이 관리한다.
 */
class FBVH8 {
    /**
     * @brief 동적 BVH가 내부 노드와 루트 저장 공간을 직접 갱신할 수 있게 합니다.
     */
    friend class FDynamicBVH8;

public:
    /**
     * @brief 노드 배열을 비우고 루트 참조를 0으로 초기화한다.
     */
    void Clear() {
        mNodes.clear();
        mRootReference = 0;
    }

    /**
     * @brief 인코딩된 루트 노드 참조를 조회한다.
     * @return 현재 루트 참조. 빈 트리의 기본값도 0이므로 노드 배열과 함께 확인한다.
     */
    Uint32 GetRootReference() const {
        return mRootReference;
    }

    /**
     * @brief BVH가 소유하는 내부 노드 배열을 조회한다.
     * @return 노드 배열의 읽기 전용 참조.
     */
    const TArray<BVH8::FNode>& GetNodes() const {
        return mNodes;
    }

    /**
     * @brief 외부 삼각형 패킷 저장소를 사용하여 최근접 교차를 검사한다.
     * @param Ray 검사할 광선.
     * @param[in,out] ClosestDistance 광선 매개변수의 비음수 상한. 교차하면 더 가까운 값으로 갱신한다.
     * @param Packets 리프 인덱스로 접근할 삼각형 패킷 배열.
     * @param ReverseWinding true이면 와인딩 판정을 뒤집어 반대 방향의 면을 검사한다.
     * @return 유효한 삼각형 교차가 있으면 true. 빈 트리나 음수 또는 NaN 상한이면 false.
     */
    bool RaycastTrianglePackets(const FRay& Ray, float& ClosestDistance, const BVH8::FTrianglePacket* Packets, bool ReverseWinding = false) const {
        if (mNodes.empty() || !(ClosestDistance >= 0.0f))
            return false;

        return BVH8::RaycastTrianglePackets(mNodes.data(), mRootReference, BVH8::FRayData{Ray}, Ray, ClosestDistance, Packets, ReverseWinding);
    }

    /**
     * @brief 기존 이진 트리의 리프 구분을 유지하면서 8분기 BVH를 구축한다.
     * @tparam TNode BoundingBox, mLeft 및 mRight를 제공하는 이진 노드 타입.
     * @tparam TIsLeaf const TNode 참조로 리프 여부를 반환하는 호출 대상.
     * @tparam TAddLeaf const TNode 참조에서 Uint32 리프 인덱스 또는 FLeaf를 반환하는 호출 대상.
     * @param BinaryNodes 인덱스 0을 루트로 하는 유효한 이진 트리.
     * @param IsLeaf 원본 노드의 리프 여부 검사.
     * @param AddLeaf 출력 리프를 저장하고 참조 정보를 반환하는 함수.
     * @details 기존 노드를 비우고 구축하며 빈 입력이면 빈 트리를 유지한다. 내부 노드를 새 리프로 합치는 최적화는 사용하지 않는다.
     */
    template <typename TNode, typename TIsLeaf, typename TAddLeaf>
    void Build(const TArray<TNode>& BinaryNodes, TIsLeaf IsLeaf, TAddLeaf AddLeaf);

    /**
     * @brief 경계 면적 기반 비용을 평가하여 이진 트리를 최대 8분기 BVH로 변환한다.
     * @tparam TNode BoundingBox, mLeft 및 mRight를 제공하는 이진 노드 타입.
     * @tparam TIsLeaf const TNode 참조로 원본 리프 여부를 검사할 타입.
     * @tparam TAddLeaf const TNode 참조에서 Uint32 인덱스 또는 FLeaf를 반환할 타입.
     * @tparam TCanMakeLeaf 내부 노드를 하나의 출력 리프로 처리할 수 있는지 검사할 타입.
     * @param BinaryNodes 인덱스 0이 루트인 유효한 이진 트리.
     * @param IsLeaf 원본 리프 검사 함수.
     * @param AddLeaf 리프를 저장하고 인덱스와 필요하면 요소 수를 반환하는 함수.
     * @param CanMakeLeaf 내부 노드를 출력 리프로 합칠 수 있는지 검사하는 함수.
     * @param LeafCost 리프 경계 면적에 곱할 비용 계수. 기본값은 1이다.
     * @details 각 하위 트리의 1부터 8까지의 분기 비용을 평가하고 유리한 구성을 선택한다. 조건과 비용이 맞으면 내부 노드도 AddLeaf()로 처리한다.
     */
    template <typename TNode, typename TIsLeaf, typename TAddLeaf, typename TCanMakeLeaf>
    void Build(const TArray<TNode>& BinaryNodes, TIsLeaf IsLeaf, TAddLeaf AddLeaf, TCanMakeLeaf CanMakeLeaf, double LeafCost = 1.0);

    /**
     * @brief 경계에 맞는 리프를 방문하여 사용자 정의 최근접 교차를 검사한다.
     * @tparam TVisitLeaf 리프 인덱스와 float 거리 참조를 받아 충돌 여부를 반환하는 호출 대상.
     * @param Ray 검사할 광선.
     * @param[in,out] ClosestDistance 광선 매개변수의 비음수 상한. 콜백이 더 가까운 교차 값으로 갱신한다.
     * @param VisitLeaf 실제 리프 교차를 검사하는 콜백.
     * @return 방문한 콜백 중 하나라도 true이면 true. 빈 트리나 음수 또는 NaN 상한이면 false.
     */
    template <typename TVisitLeaf>
    bool Raycast(const FRay& Ray, float& ClosestDistance, TVisitLeaf VisitLeaf) const;

private:
    /**
     * @brief 구축된 8분기 내부 노드 배열.
     */
    TArray<BVH8::FNode> mNodes;
    /**
     * @brief 현재 트리의 인코딩된 루트 참조.
     */
    Uint32 mRootReference = 0;
};

template <typename TNode, typename TIsLeaf, typename TAddLeaf>
void FBVH8::Build(const TArray<TNode>& BinaryNodes, TIsLeaf IsLeaf, TAddLeaf AddLeaf) {
    Build(BinaryNodes, IsLeaf, AddLeaf, [](const TNode&) {
        return false;
    });
}

template <typename TNode, typename TIsLeaf, typename TAddLeaf, typename TCanMakeLeaf>
void FBVH8::Build(const TArray<TNode>& BinaryNodes, TIsLeaf IsLeaf, TAddLeaf AddLeaf, TCanMakeLeaf CanMakeLeaf, double LeafCost) {
    Clear();

    if (BinaryNodes.empty()) {
        return;
    }

    const auto Area{[](const DirectX::BoundingBox& Box) {
        return static_cast<double>(Box.Extents.x) * Box.Extents.y + static_cast<double>(Box.Extents.y) * Box.Extents.z + static_cast<double>(Box.Extents.z) * Box.Extents.x;
    }};

    /**
     * @brief 이진 하위 트리를 1부터 8개 경계로 확장할 때의 비용과 최적 분할을 저장하는 구축 계획.
     */
    struct FPlan {
        /**
         * @brief 인덱스 1부터 8까지의 분기 개수별 비용. 사용할 수 없는 구성은 무한대로 표시한다.
         */
        std::array<double, 9> mCost{};
        /**
         * @brief 각 분기 개수의 최소 비용 구성에서 왼쪽 하위 트리에 배분한 개수.
         */
        std::array<Uint8, 9> mLeftCount{};
        /**
         * @brief 현재 하위 트리에 선택한 출력 경계 개수.
         */
        Uint8 mBestCount{1};
    };

    TArray<FPlan> Plans{BinaryNodes.size()};

    const auto Evaluate{[&](auto&& Self, Uint32 Index) -> void {
        const TNode& Node{BinaryNodes[Index]};
        FPlan& Plan{Plans[Index]};

        Plan.mCost.fill(std::numeric_limits<double>::infinity());

        if (IsLeaf(Node)) {
            Plan.mCost[1] = Area(Node.BoundingBox) * LeafCost;
            return;
        }

        Self(Self, Node.mLeft);
        Self(Self, Node.mRight);

        double BestCost{std::numeric_limits<double>::infinity()};

        for (Uint8 Count{2}; Count <= 8; ++Count) {
            for (Uint8 Left{1}; Left < Count; ++Left) {
                const double Cost{Plans[Node.mLeft].mCost[Left] + Plans[Node.mRight].mCost[Count - Left]};

                if (Cost < Plan.mCost[Count]) {
                    Plan.mCost[Count] = Cost;
                    Plan.mLeftCount[Count] = Left;
                }
            }

            if (Plan.mLeftCount[Count] != 0 && Plan.mCost[Count] <= BestCost) {
                BestCost = Plan.mCost[Count];
                Plan.mBestCount = Count;
            }
        }

        Plan.mCost[1] = Area(Node.BoundingBox) + BestCost;

        if (CanMakeLeaf(Node) && Area(Node.BoundingBox) * LeafCost <= Plan.mCost[1]) {
            Plan.mCost[1] = Area(Node.BoundingBox) * LeafCost;
            Plan.mBestCount = 1;
        }
    }};

    Evaluate(Evaluate, 0);

    const auto Gather{[&](auto&& Self, Uint32 Index, Uint8 Count, Uint32* Frontier, Uint32& Size) -> void {
        if (Count == 1) {
            Frontier[Size++] = Index;
            return;
        }

        const TNode& Node{BinaryNodes[Index]};
        const Uint8 Left{Plans[Index].mLeftCount[Count]};

        Self(Self, Node.mLeft, Left, Frontier, Size);
        Self(Self, Node.mRight, Count - Left, Frontier, Size);
    }};

    const auto Collapse{[&](auto&& Self, Uint32 BinaryIndex) -> Uint32 {
        Uint32 Frontier[8]{};
        Uint32 Count{};

        Gather(Gather, BinaryIndex, Plans[BinaryIndex].mBestCount, Frontier, Count);
        ErrorHandler::Report(mNodes.size() > BVH8::IndexMask, "FBVH8::Build", "BVH8 node capacity exceeded.", ErrorHandler::EErrorLevel::Critical);

        const Uint32 Index{static_cast<Uint32>(mNodes.size())};

        mNodes.emplace_back();

        for (Uint32 Lane{}; Lane < Count; ++Lane) {
            const TNode& Source{BinaryNodes[Frontier[Lane]]};
            const Uint32 Child{Plans[Frontier[Lane]].mBestCount == 1 ? BVH8::MakeLeafReference(AddLeaf(Source)) : Self(Self, Frontier[Lane])};
            BVH8::FNode& Node{mNodes[Index]};
            const DirectX::BoundingBox& Box{Source.BoundingBox};

            Node.MinX[Lane] = Box.Center.x - Box.Extents.x;
            Node.MaxX[Lane] = Box.Center.x + Box.Extents.x;
            Node.MinY[Lane] = Box.Center.y - Box.Extents.y;
            Node.MaxY[Lane] = Box.Center.y + Box.Extents.y;
            Node.MinZ[Lane] = Box.Center.z - Box.Extents.z;
            Node.MaxZ[Lane] = Box.Center.z + Box.Extents.z;
            Node.Children[Lane] = Child;
        }

        return BVH8::MakeReference(Index, Count);
    }};

    mRootReference = Collapse(Collapse, 0);
    mNodes.shrink_to_fit();
}

template <typename TVisitLeaf>
bool FBVH8::Raycast(const FRay& Ray, float& ClosestDistance, TVisitLeaf VisitLeaf) const {
    if (mNodes.empty() || !(ClosestDistance >= 0.0f)) {
        return false;
    }

    const BVH8::FRayData RayData{Ray};

    return BVH8::Raycast(mNodes.data(), mRootReference, RayData, ClosestDistance, &VisitLeaf, [](void* Context, Uint32 Leaf, float& Distance) {
        return (*static_cast<TVisitLeaf*>(Context))(Leaf, Distance);
    });
}
