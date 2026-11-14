/**
 * @file FMeshData.cpp
 * @brief 메시 정점 속성·인덱스의 검증과 CPU 데이터 처리에 대한 구현입니다.
 * @ingroup Asset
 */

#include "Core/Types.h"
#include "Asset/FMeshData.h"

namespace Asset {

    /**
     * @brief 정점별 위치 벡터 배열의 참조를 반환합니다.
     */
    Core::TArray<Core::FVector>& Asset::FMeshData::GetPositions() {
        return mPositions;
    }

    /**
     * @brief 정점별 위치 벡터 배열의 참조를 반환합니다.
     */
    const Core::TArray<Core::FVector>& Asset::FMeshData::GetPositions() const {
        return mPositions;
    }

    /**
     * @brief 정점별 법선 벡터 배열의 참조를 반환합니다.
     */
    Core::TArray<Core::FVector>& Asset::FMeshData::GetNormals() {
        return mNormals;
    }

    /**
     * @brief 정점별 법선 벡터 배열의 참조를 반환합니다.
     */
    const Core::TArray<Core::FVector>& Asset::FMeshData::GetNormals() const {
        return mNormals;
    }

    /**
     * @brief 정점별 탄젠트와 방향 부호 배열의 참조를 반환합니다.
     */
    Core::TArray<Core::FVector4>& Asset::FMeshData::GetTangents() {
        return mTangents;
    }

    /**
     * @brief 정점별 탄젠트와 방향 부호 배열의 참조를 반환합니다.
     */
    const Core::TArray<Core::FVector4>& Asset::FMeshData::GetTangents() const {
        return mTangents;
    }

    /**
     * @brief 채널별 정점 UV 좌표 배열의 참조를 반환합니다.
     */
    Core::TArray<Core::TArray<Core::FVector2>>& Asset::FMeshData::GetUVChannels() {
        return mUVChannels;
    }

    /**
     * @brief 채널별 정점 UV 좌표 배열의 참조를 반환합니다.
     */
    const Core::TArray<Core::TArray<Core::FVector2>>& Asset::FMeshData::GetUVChannels() const {
        return mUVChannels;
    }

    /**
     * @brief 정점별 색상 배열의 참조를 반환합니다.
     */
    Core::TArray<Core::FVector4>& Asset::FMeshData::GetColors() {
        return mColors;
    }

    /**
     * @brief 정점별 색상 배열의 참조를 반환합니다.
     */
    const Core::TArray<Core::FVector4>& Asset::FMeshData::GetColors() const {
        return mColors;
    }

    /**
     * @brief 삼각형을 구성하는 정점 인덱스 배열의 참조를 반환합니다.
     */
    Core::TArray<Core::Index_t>& Asset::FMeshData::GetIndices() {
        return mIndices;
    }

    /**
     * @brief 삼각형을 구성하는 정점 인덱스 배열의 참조를 반환합니다.
     */
    const Core::TArray<Core::Index_t>& Asset::FMeshData::GetIndices() const {
        return mIndices;
    }

    /**
     * @brief 메시의 로컬 공간 경계 상자를 반환합니다.
     */
    const Core::FBox& Asset::FMeshData::GetBounds() const {
        return mBounds;
    }

    /**
     * @brief 메시의 로컬 공간 경계 상자를 교체합니다.
     */
    void Asset::FMeshData::SetBounds(const Core::FBox& Bounds) {
        mBounds = Bounds;
    }

    /**
     * @brief 현재 정점 위치를 모두 감싸는 로컬 공간 경계 상자를 계산해 설정합니다.
     */
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

    /**
     * @brief 재질별 인덱스 구간을 반환합니다.
     */
    Core::TArray<Asset::FSubMesh>& Asset::FMeshData::GetSubMeshes() {
        return mSubMeshes;
    }

    /**
     * @brief 재질별 인덱스 구간을 반환합니다.
     */
    const Core::TArray<Asset::FSubMesh>& Asset::FMeshData::GetSubMeshes() const {
        return mSubMeshes;
    }

    /**
     * @brief 재질 슬롯을 반환합니다.
     */
    Core::TArray<Asset::FMeshMaterialSlot>& Asset::FMeshData::GetMaterialSlots() {
        return mMaterialSlots;
    }

    /**
     * @brief 재질 슬롯을 반환합니다.
     */
    const Core::TArray<Asset::FMeshMaterialSlot>& Asset::FMeshData::GetMaterialSlots() const {
        return mMaterialSlots;
    }

    /**
     * @brief 정점 속성과 인덱스 배열에 할당된 CPU 메모리 바이트 수를 반환합니다.
     */
    std::size_t Asset::FMeshData::GetAllocatedBytes() const {
        std::size_t Bytes{mPositions.capacity() * sizeof(Core::FVector) + mNormals.capacity() * sizeof(Core::FVector) + mTangents.capacity() * sizeof(Core::FVector4) + mUVChannels.capacity() * sizeof(Core::TArray<Core::FVector2>) + mColors.capacity() * sizeof(Core::FVector4) + mIndices.capacity() * sizeof(Core::Index_t)};

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
