#include "pch.h"
#include "Asset/UMaterial.h"

void UMaterial::BuildGPUData(Uint32 GroupIndex, FMaterialGPUSlot& OutSlot) const {
    if (GroupIndex != 0) {
        OutSlot = {};
        return;
    }

    BuildGPUData(OutSlot);
}

FMaterialChunkSignature UMaterial::BuildChunkSignature() const {
    return {};
}

FMaterialChunkSignature UMaterial::BuildChunkSignature(Uint32 GroupIndex) const {
    return GroupIndex == 0 ? BuildChunkSignature() : FMaterialChunkSignature{};
}

void UMaterial::Finalize(const IAssetRegistry* Query) {
}

std::optional<Uint32> UMaterial::FindGroupIndex(FName Name) const {
    return std::nullopt;
}

void UMaterial::Serialize(FArchive& Ar) {
    UAsset::Serialize(Ar);
}

Uint32 UMaterial::GetGPUDataCount() const {
    return 1;
}

EMaterialBlendMode UMaterial::GetBlendMode(Uint32 GroupIndex) const {
    return EMaterialBlendMode::Opaque;
}

void UMaterial::MarkGPUDataDirty() {
    ++mRenderRevision;
}

Uint64 UMaterial::GetRenderRevision() const {
    return mRenderRevision;
}
