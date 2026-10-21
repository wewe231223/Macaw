#pragma once

#include "Core/CoreMinimal.h"
#include "Core/Base/FName.h"
#include "Core/Asset/FAssetReference.h"

class FArchive;
class IAssetResolver;

struct FMeshSection {
    Uint32 mFirstIndex{};
    Uint32 mIndexCount{};
    Uint32 mMaterialSlotIndex{};
    Uint32 mSourceIndexCount{};
};

struct FMeshMaterialSlot {
    FName mName{};
    FAssetReference mMaterial{};

    void Serialize(FArchive& Archive);
};

struct FMeshData {
    TArray<FVector3> mPositions{};
    TArray<FVector3> mNormals{};
    TArray<FVector2> mUVs{};
    TArray<FColor4> mColors{};
    TArray<Uint32> mIndices{};
    TArray<FMeshSection> mSections{};
    TArray<FMeshMaterialSlot> mMaterialSlots{};
};
