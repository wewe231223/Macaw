#include "pch.h"
#include "RenderCore/FMaterialData.h"

namespace {
    bool IsUnitValue(float Value) {
        return std::isfinite(Value) && Value >= 0.0f && Value <= 1.0f;
    }

    bool IsPositiveColor(const FVector3& Value) {
        return std::isfinite(Value.mX) && std::isfinite(Value.mY) && std::isfinite(Value.mZ) && Value.mX >= 0.0f && Value.mY >= 0.0f && Value.mZ >= 0.0f;
    }
}

bool IsValidMaterialData(const FMaterialGPUData& Data) {
    return IsPositiveColor(Data.mBaseColor) && IsPositiveColor(Data.mEmissive) && IsUnitValue(Data.mOpacity) && std::isfinite(Data.mNormalScale) && Data.mNormalScale >= 0.0f && IsUnitValue(Data.mMetallic) && IsUnitValue(Data.mRoughness) && IsUnitValue(Data.mSpecular) && IsUnitValue(Data.mAmbientOcclusion) && IsUnitValue(Data.mOpacityMaskClipValue) && Data.mFlags == 0 && Data.mExtensionOffset == 0 && Data.mExtensionLayoutId == 0;
}
