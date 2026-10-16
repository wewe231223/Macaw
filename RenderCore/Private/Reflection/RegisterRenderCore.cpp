#include "pch.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "RenderCore/FMaterialBlendMode.h"
#include "RenderCore/FMaterialData.h"
#include "RenderCore/FRenderData.h"
#include "RenderCore/FUVTransform.h"

void ReflectionModules::RegisterRenderCore() {
    StaticEnumEMaterialBlendMode();
    FMaterialGPUData::StaticStruct();
    FTextVertex::StaticStruct();
    FPostProcessingSettings::StaticStruct();
    FRenderSettings::StaticStruct();
    StaticEnumELightType();
    FUVTransform::StaticStruct();
}
