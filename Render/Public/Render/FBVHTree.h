#pragma once

#include <DirectXCollision.h>
#include "RenderCore/FRenderData.h"

#include <array>
#include <span>

struct FBVHNode {
    DirectX::BoundingBox mBounds{};
    Int32 mLeftChild{-1};
    Int32 mRightChild{-1};
    Int32 mObjectIndex{-1};
    Int32 mParentIndex{-1};

    Uint32 mFirstIndex{};
    Uint32 mIndexCount{};

    bool IsLeaf() const;
};

class FBVHTree {
public:
    FBVHTree() = default;

public:
    void Build(std::span<const DirectX::BoundingBox> Bounds, std::span<const Uint32> Indices);
    void Clear();

    const TArray<FBVHNode>& GetNodes() const;

    void UpdateBounds(Uint32 ObjectIndex, const DirectX::BoundingBox& Bounds);
    void Refit(std::span<const Uint32> ChangedIndices);

    void FrustumCull(const FFrustum& Frustum, TArray<Uint32>& OutIndices) const;
    void FrustumCull(const FMatrix& ViewProjection, TArray<Uint32>& OutIndices, TArray<Uint32>* OutBoundaryPositions = nullptr) const;

private:
    Int32 BuildRecursive(std::span<const DirectX::BoundingBox> Bounds, std::size_t Start, std::size_t End, Int32 ParentIndex);

    void CullRecursive(Int32 NodeIndex, const FFrustum& Frustum, TArray<Uint32>& OutIndices) const;
    void CullRecursive(Int32 NodeIndex, const std::array<DirectX::XMFLOAT4, 6>& Planes, TArray<Uint32>& OutIndices, TArray<Uint32>* OutBoundaryPositions) const;

    void CollectAllLeaves(Int32 NodeIndex, TArray<Uint32>& OutIndices) const;

private:
    TArray<FBVHNode> mNodes{};
    Int32 mRootIndex{-1};

    TArray<Uint32> mIndices{};
    TArray<Int32> mObjectLeaves{};
};
