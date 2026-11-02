/**
 * @file FMeshData.cpp
 * @brief 메시 정점 속성·인덱스의 검증과 CPU 데이터 처리에 대한 구현입니다.
 * @ingroup Asset
 */

#include "Asset/FMeshData.h"

namespace Asset {

    Core::TArray<Core::FVector>& Asset::FMeshData::GetPositions() {
        return mPositions;
    }

    const Core::TArray<Core::FVector>& Asset::FMeshData::GetPositions() const {
        return mPositions;
    }

    Core::TArray<Core::FVector>& Asset::FMeshData::GetNormals() {
        return mNormals;
    }

    const Core::TArray<Core::FVector>& Asset::FMeshData::GetNormals() const {
        return mNormals;
    }

    Core::TArray<Core::FVector4>& Asset::FMeshData::GetTangents() {
        return mTangents;
    }

    const Core::TArray<Core::FVector4>& Asset::FMeshData::GetTangents() const {
        return mTangents;
    }

    Core::TArray<Core::TArray<Core::FVector2>>& Asset::FMeshData::GetUVChannels() {
        return mUVChannels;
    }

    const Core::TArray<Core::TArray<Core::FVector2>>& Asset::FMeshData::GetUVChannels() const {
        return mUVChannels;
    }

    Core::TArray<Core::FVector4>& Asset::FMeshData::GetColors() {
        return mColors;
    }

    const Core::TArray<Core::FVector4>& Asset::FMeshData::GetColors() const {
        return mColors;
    }

    Core::TArray<Core::Uint32>& Asset::FMeshData::GetIndices() {
        return mIndices;
    }

    const Core::TArray<Core::Uint32>& Asset::FMeshData::GetIndices() const {
        return mIndices;
    }

    const Core::FBox& Asset::FMeshData::GetBounds() const {
        return mBounds;
    }

    void Asset::FMeshData::SetBounds(const Core::FBox& Bounds) {
        mBounds = Bounds;
    }

    void Asset::FMeshData::ComputeBounds() {
        if (mPositions.empty()) {
            mBounds = {};
            return;
        }

        Core::FVector Minimum{mPositions.front()};
        Core::FVector Maximum{Minimum};

        for (const Core::FVector& Position : mPositions) {
            Minimum = Minimum.Min(Position);
            Maximum = Maximum.Max(Position);
        }

        mBounds = {Minimum, Maximum};
    }

    Core::TArray<Asset::FSubMesh>& Asset::FMeshData::GetSubMeshes() {
        return mSubMeshes;
    }

    const Core::TArray<Asset::FSubMesh>& Asset::FMeshData::GetSubMeshes() const {
        return mSubMeshes;
    }

    Core::TArray<Asset::FMeshMaterialSlot>& Asset::FMeshData::GetMaterialSlots() {
        return mMaterialSlots;
    }

    const Core::TArray<Asset::FMeshMaterialSlot>& Asset::FMeshData::GetMaterialSlots() const {
        return mMaterialSlots;
    }

    std::size_t Asset::FMeshData::GetAllocatedBytes() const {
        std::size_t Bytes{mPositions.capacity() * sizeof(Core::FVector) + mNormals.capacity() * sizeof(Core::FVector) + mTangents.capacity() * sizeof(Core::FVector4) + mUVChannels.capacity() * sizeof(Core::TArray<Core::FVector2>) + mColors.capacity() * sizeof(Core::FVector4) + mIndices.capacity() * sizeof(Core::Uint32)};

        for (const Core::TArray<Core::FVector2>& Channel : mUVChannels) {
            Bytes += Channel.capacity() * sizeof(Core::FVector2);
        }

        Bytes += mSubMeshes.capacity() * sizeof(Asset::FSubMesh) + mMaterialSlots.capacity() * sizeof(Asset::FMeshMaterialSlot);

        for (const Asset::FMeshMaterialSlot& Slot : mMaterialSlots) {
            Bytes += Slot.GetName().capacity() + Slot.GetMaterialPath().capacity();
        }

        return Bytes;
    }

}
