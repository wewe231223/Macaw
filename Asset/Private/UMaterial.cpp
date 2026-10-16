#include "pch.h"
#include "Asset/UMaterial.h"
#include "Asset/FMaterialParameter.h"

void UMaterial::BuildGPUData(FMaterialGPUSlot& OutSlot) const {
    OutSlot = {};
}

bool UMaterial::BuildResolvedData(const IAssetRegistry& Registry, FResolvedMaterialData& OutData) const {
    OutData = {};
    return false;
}

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
    Super::Serialize(Ar);
}

FName UMaterial::GetGroupName(Uint32 GroupIndex) const {
    return {};
}

Uint32 UMaterial::GetGPUDataCount() const {
    return 0;
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

UMaterial::UMaterial() {
    InitializeClass(StaticClass());
}

UMaterial::~UMaterial() = default;
