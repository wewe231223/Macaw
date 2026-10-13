#include "pch.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Asset/FMaterialGroup.h"
#include "Asset/Pipeline/UPipeline.h"
#include "Asset/UAsset.h"
#include "Asset/UFont.h"
#include "Asset/UFreeTypeFont.h"
#include "Asset/UMaterial.h"
#include "Asset/UMesh.h"
#include "Asset/USurfaceOpaque.h"
#include "Asset/UTexture.h"

void ReflectionModules::RegisterAsset() {
    FMaterialTextureMap::StaticStruct();
    FMaterialGroup::StaticStruct();
    UPipeline::StaticClass();
    UAsset::StaticClass();
    UFont::StaticClass();
    UFreeTypeFont::StaticClass();
    UMaterial::StaticClass();
    UMesh::StaticClass();
    USurfaceOpaque::StaticClass();
    UTexture::StaticClass();
}
