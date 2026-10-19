#include "pch.h"
#include "RegisterAsset.h"
#include "Asset/FSurfaceParameters.h"
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
    FSurfaceParameters::StaticStruct();
    UPipeline::StaticClass();
    UAsset::StaticClass();
    UFont::StaticClass();
    UFreeTypeFont::StaticClass();
    UMaterial::StaticClass();
    UMesh::StaticClass();
    USurfaceOpaque::StaticClass();
    UTexture::StaticClass();
}
