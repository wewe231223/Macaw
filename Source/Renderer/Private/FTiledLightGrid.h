#pragma once

#include "Engine/FLightSceneProxy.h"
#include "Engine/FSceneView.h"

#include <span>

struct FGpuLight {
public:
    ReflectTypes(FGpuLight);

public:
    ReflectStruct(FGpuLight);

private:
    ReflectMembers();

public:
    FVector4 mPositionAndRadius{};
    ReflectProperty(mPositionAndRadius, "PositionAndRadius", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mDirectionAndType{};
    ReflectProperty(mDirectionAndType, "DirectionAndType", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mColorAndIntensity{};
    ReflectProperty(mColorAndIntensity, "ColorAndIntensity", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mFalloffAndCones{};
    ReflectProperty(mFalloffAndCones, "FalloffAndCones", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

class FTiledLightGrid {
public:
    ReflectTypes(FTiledLightGrid);

public:
    ReflectStruct(FTiledLightGrid);

public:
    void Build(std::span<const FLightSceneProxy> Lights, const FSceneView& View);
    const TArray<FGpuLight>& GetLights() const;
    const TArray<std::array<Uint32, 2>>& GetRanges() const;
    const TArray<Uint32>& GetIndices() const;
    Uint32 GetColumns() const;
    Uint32 GetRows() const;

private:
    ReflectMembers();

private:
    TArray<FGpuLight> mLights{};
    ReflectProperty(mLights, "Lights", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<std::array<Uint32, 2>> mRanges{};
    ReflectProperty(mRanges, "Ranges", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<Uint32> mIndices{};
    ReflectProperty(mIndices, "Indices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mColumns{};
    ReflectProperty(mColumns, "Columns", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mRows{};
    ReflectProperty(mRows, "Rows", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
