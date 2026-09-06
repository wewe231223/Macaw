#include "PCH.h"
#include "UMaterial.h"

bool UMaterial::Initialize(ID3D11Device* Device) {
    return true;
}

void UMaterial::BuildGPUData(FMaterialGPUSlot& OutSlot) const {
    OutSlot = {};
}