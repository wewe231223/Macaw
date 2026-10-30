/**
 * @file FMeshData.cpp
 * @brief 메시 정점 속성·인덱스의 검증과 CPU 데이터 처리에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/FMeshData.h"

namespace Engine {

    Core::TArray<Core::FVector>& Engine::FMeshData::GetPositions() {
        return mPositions;
    }

    const Core::TArray<Core::FVector>& Engine::FMeshData::GetPositions() const {
        return mPositions;
    }

    Core::TArray<Core::FVector>& Engine::FMeshData::GetNormals() {
        return mNormals;
    }

    const Core::TArray<Core::FVector>& Engine::FMeshData::GetNormals() const {
        return mNormals;
    }

    Core::TArray<Core::FVector4>& Engine::FMeshData::GetTangents() {
        return mTangents;
    }

    const Core::TArray<Core::FVector4>& Engine::FMeshData::GetTangents() const {
        return mTangents;
    }

    Core::TArray<Core::TArray<Core::FVector2>>& Engine::FMeshData::GetUVChannels() {
        return mUVChannels;
    }

    const Core::TArray<Core::TArray<Core::FVector2>>& Engine::FMeshData::GetUVChannels() const {
        return mUVChannels;
    }

    Core::TArray<Core::FVector4>& Engine::FMeshData::GetColors() {
        return mColors;
    }

    const Core::TArray<Core::FVector4>& Engine::FMeshData::GetColors() const {
        return mColors;
    }

    Core::TArray<Core::Uint32>& Engine::FMeshData::GetIndices() {
        return mIndices;
    }

    const Core::TArray<Core::Uint32>& Engine::FMeshData::GetIndices() const {
        return mIndices;
    }

    const Core::FBox& Engine::FMeshData::GetBounds() const {
        return mBounds;
    }

    void Engine::FMeshData::SetBounds(const Core::FBox& Bounds) {
        mBounds = Bounds;
    }

    Core::TArray<Engine::FSubMesh>& Engine::FMeshData::GetSubMeshes() {
        return mSubMeshes;
    }

    const Core::TArray<Engine::FSubMesh>& Engine::FMeshData::GetSubMeshes() const {
        return mSubMeshes;
    }

    Core::TArray<Engine::FMeshMaterialSlot>& Engine::FMeshData::GetMaterialSlots() {
        return mMaterialSlots;
    }

    const Core::TArray<Engine::FMeshMaterialSlot>& Engine::FMeshData::GetMaterialSlots() const {
        return mMaterialSlots;
    }

    std::size_t Engine::FMeshData::GetAllocatedBytes() const {
        std::size_t Bytes{mPositions.capacity() * sizeof(Core::FVector) + mNormals.capacity() * sizeof(Core::FVector) + mTangents.capacity() * sizeof(Core::FVector4) + mUVChannels.capacity() * sizeof(Core::TArray<Core::FVector2>) + mColors.capacity() * sizeof(Core::FVector4) + mIndices.capacity() * sizeof(Core::Uint32)};

        for (const Core::TArray<Core::FVector2>& Channel : mUVChannels) {
            Bytes += Channel.capacity() * sizeof(Core::FVector2);
        }

        Bytes += mSubMeshes.capacity() * sizeof(Engine::FSubMesh) + mMaterialSlots.capacity() * sizeof(Engine::FMeshMaterialSlot);

        for (const Engine::FMeshMaterialSlot& Slot : mMaterialSlots) {
            Bytes += Slot.GetName().capacity() + Slot.GetMaterialPath().capacity();
        }

        return Bytes;
    }

}
