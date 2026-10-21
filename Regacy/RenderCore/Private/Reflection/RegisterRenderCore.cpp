#include "pch.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "RenderCore/FMaterialBlendMode.h"
#include "RenderCore/FRenderData.h"
#include "RenderCore/FUVTransform.h"

void ReflectionModules::RegisterRenderCore() {
    StaticEnumEMaterialBlendMode();
    FTextVertex::StaticStruct();
    FPostProcessingSettings::StaticStruct();
    FRenderSettings::StaticStruct();
    StaticEnumELightType();
    FUVTransform::StaticStruct();
}
