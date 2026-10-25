#include "Engine/FMeshData.h"

TArray<FVector>& FMeshData::GetPositions() {
    return mPositions;
}

const TArray<FVector>& FMeshData::GetPositions() const {
    return mPositions;
}

TArray<FVector>& FMeshData::GetNormals() {
    return mNormals;
}

const TArray<FVector>& FMeshData::GetNormals() const {
    return mNormals;
}

TArray<FVector4>& FMeshData::GetTangents() {
    return mTangents;
}

const TArray<FVector4>& FMeshData::GetTangents() const {
    return mTangents;
}

TArray<TArray<FVector2>>& FMeshData::GetUVChannels() {
    return mUVChannels;
}

const TArray<TArray<FVector2>>& FMeshData::GetUVChannels() const {
    return mUVChannels;
}

TArray<FVector4>& FMeshData::GetColors() {
    return mColors;
}

const TArray<FVector4>& FMeshData::GetColors() const {
    return mColors;
}

TArray<Uint32>& FMeshData::GetIndices() {
    return mIndices;
}

const TArray<Uint32>& FMeshData::GetIndices() const {
    return mIndices;
}

const FBox& FMeshData::GetBounds() const {
    return mBounds;
}

void FMeshData::SetBounds(const FBox& Bounds) {
    mBounds = Bounds;
}

std::size_t FMeshData::GetAllocatedBytes() const {
    std::size_t Bytes{mPositions.capacity() * sizeof(FVector) + mNormals.capacity() * sizeof(FVector) + mTangents.capacity() * sizeof(FVector4) + mUVChannels.capacity() * sizeof(TArray<FVector2>) + mColors.capacity() * sizeof(FVector4) + mIndices.capacity() * sizeof(Uint32)};

    for (const TArray<FVector2>& Channel : mUVChannels) {
        Bytes += Channel.capacity() * sizeof(FVector2);
    }

    return Bytes;
}
