#include "pch.h"
#include "Asset/FMeshData.h"
#include "CoreUObject/Reflection/NativeSerialization.h"

void FMeshMaterialSlot::Serialize(FArchive& Archive) {
    Archive.Serialize("Name", mName);
    SerializeAssetReference(Archive, "MaterialGuid", "MaterialPath", mMaterial);
}
