#include "PCH.h"
#include "UColorMaterial.h"

#include <cstring>

bool UColorMaterial::Initialize(ID3D11Device* Device) {
    return true;
}

bool UColorMaterial::Initialize(ID3D11Device* Device, const FVector4& InColor) {
    Color = InColor;
    UMaterial::MarkGPUDataDirty();

    return true;
}

void UColorMaterial::BuildGPUData(FMaterialGPUSlot& OutSlot) const {
    FColorMaterialGPUData Data{};
    Data.Color = Color;

    std::memcpy(OutSlot.Data.data(), &Data, sizeof(FColorMaterialGPUData));
}

void UColorMaterial::SetColor(const FVector4& InColor) {
    Color = InColor;
    UMaterial::MarkGPUDataDirty();
}