#include "pch.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Asset/FMaterialGroup.h"
#include "Asset/FMaterialParameter.h"
#include "Asset/Pipeline/UPipeline.h"
#include "Asset/UAsset.h"
#include "Asset/UFont.h"
#include "Asset/UFreeTypeFont.h"
#include "Asset/UMaterial.h"
#include "Asset/UMaterialDefinition.h"
#include "Asset/UMaterialInstance.h"
#include "Asset/UMesh.h"
#include "Asset/USurfaceOpaque.h"
#include "Asset/UTexture.h"

void ReflectionModules::RegisterAsset() {
    FMaterialTextureMap::StaticStruct();
    FMaterialGroup::StaticStruct();
    FMaterialAssetReference::StaticStruct();
    FMaterialParameterValue::StaticStruct();
    FMaterialParameterDefinition::StaticStruct();
    FMaterialParameterOverride::StaticStruct();
    FMaterialCoreOverride::StaticStruct();
    FResolvedMaterialData::StaticStruct();
    StaticEnumEMaterialParameterType();
    StaticEnumEMaterialCoreParameter();
    UPipeline::StaticClass();
    UAsset::StaticClass();
    UFont::StaticClass();
    UFreeTypeFont::StaticClass();
    UMaterial::StaticClass();
    UMaterialDefinition::StaticClass();
    UMaterialInstance::StaticClass();
    UMesh::StaticClass();
    USurfaceOpaque::StaticClass();
    UTexture::StaticClass();
}
