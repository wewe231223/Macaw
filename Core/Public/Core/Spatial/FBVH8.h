#pragma once
#include "Core/Base/ErrorHandler.h"
#include "Core/Common.h"
#include "Math/FMath.h"
#include <array>
#include <bit>
#include <limits>

namespace BVH8 {
    constexpr Uint32 LeafBit = 0x80000000u;
    constexpr Uint32 CountShift = 28;
    constexpr Uint32 IndexMask = (1u << CountShift) - 1;

    constexpr Uint32 GetCount(Uint32 Reference) {
        return ((Reference >> CountShift) & 7u) + 1;
    }

    constexpr Uint32 GetValidMask(Uint32 Reference) {
        return (1u << GetCount(Reference)) - 1;
    }

    Uint32 MakeReference(Uint32 Index, Uint32 Count, bool Leaf = false);

    struct FLeaf {
        Uint32 Index, Count;
    };

    inline Uint32 MakeLeafReference(FLeaf Leaf) {
        return MakeReference(Leaf.Index, Leaf.Count, true);
    }

    inline Uint32 MakeLeafReference(Uint32 Index) {
        return MakeReference(Index, 1, true);
    }

    struct alignas(32) FNode {
        float MinX[8]{}, MaxX[8]{};
        float MinY[8]{}, MaxY[8]{};
        float MinZ[8]{}, MaxZ[8]{};
        Uint32 Children[8]{};
    };

    static_assert(sizeof(FNode) == 224 && alignof(FNode) == 32);

    struct FRayData {
        float Origin[3];
        float InverseDirection[3];
        bool Parallel[3];

        explicit FRayData(const FRay& Ray);
    };

    using FVisitLeaf = bool (*)(void*, Uint32, float&);
    using FRaycaster = bool (*)(const FNode*, Uint32, const FRayData&, float&, void*, FVisitLeaf);
    struct FTrianglePacket;
    using FTrianglePacketRaycaster = bool (*)(const FNode*, Uint32, const FRayData&, const FRay&, float&, const FTrianglePacket*, bool);
    void Initialize();
    bool Raycast(const FNode* Nodes, Uint32 RootReference, const FRayData& Ray, float& ClosestDistance, void* Context, FVisitLeaf VisitLeaf);
    bool RaycastSSE(const FNode* Nodes, Uint32 RootReference, const FRayData& Ray, float& ClosestDistance, void* Context, FVisitLeaf VisitLeaf);
    bool RaycastAVX(const FNode* Nodes, Uint32 RootReference, const FRayData& Ray, float& ClosestDistance, void* Context, FVisitLeaf VisitLeaf);
    bool SupportsAVX();
    bool RaycastTrianglePackets(const FNode* Nodes, Uint32 RootReference, const FRayData& RayData, const FRay& Ray, float& ClosestDistance, const FTrianglePacket* Packets, bool ReverseWinding = false);
    bool RaycastTrianglePacketsSSE(const FNode* Nodes, Uint32 RootReference, const FRayData& RayData, const FRay& Ray, float& ClosestDistance, const FTrianglePacket* Packets, bool ReverseWinding = false);
    bool RaycastTrianglePacketsAVX(const FNode* Nodes, Uint32 RootReference, const FRayData& RayData, const FRay& Ray, float& ClosestDistance, const FTrianglePacket* Packets, bool ReverseWinding = false);
}

class FBVH8 {
    friend class FDynamicBVH8;

public:
    void Clear() {
        mNodes.clear();
        mRootReference = 0;
    }

    Uint32 GetRootReference() const {
        return mRootReference;
    }

    const TArray<BVH8::FNode>& GetNodes() const {
        return mNodes;
    }

    bool RaycastTrianglePackets(const FRay& Ray, float& ClosestDistance, const BVH8::FTrianglePacket* Packets, bool ReverseWinding = false) const {
        if (mNodes.empty() || !(ClosestDistance >= 0.0f))
            return false;

        return BVH8::RaycastTrianglePackets(mNodes.data(), mRootReference, BVH8::FRayData{Ray}, Ray, ClosestDistance, Packets, ReverseWinding);
    }

    template <typename TNode, typename TIsLeaf, typename TAddLeaf>
    void Build(const TArray<TNode>& BinaryNodes, TIsLeaf IsLeaf, TAddLeaf AddLeaf);

    template <typename TNode, typename TIsLeaf, typename TAddLeaf, typename TCanMakeLeaf>
    void Build(const TArray<TNode>& BinaryNodes, TIsLeaf IsLeaf, TAddLeaf AddLeaf, TCanMakeLeaf CanMakeLeaf, double LeafCost = 1.0);

    template <typename TVisitLeaf>
    bool Raycast(const FRay& Ray, float& ClosestDistance, TVisitLeaf VisitLeaf) const;

private:
    TArray<BVH8::FNode> mNodes;
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

    struct FPlan {
        std::array<double, 9> mCost{};
        std::array<Uint8, 9> mLeftCount{};
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
