#ifndef MacawPixelLightingIncluded
#define MacawPixelLightingIncluded

#include "LightingCommon.hlsli"

#if defined(MACAW_RECEIVE_SHADOWS)
#include "ShadowSampling.hlsli"
#endif

StructuredBuffer<uint2> TileRanges : register(t3);
StructuredBuffer<uint> TileLightIndices : register(t4);

FLighting AccumulatePixelLighting(float2 Pixel, float3 Position, float3 Normal, float3 AmbientTint, float2 SpecularParameters, bool ReceiveShadows, float3 PositionDx, float3 PositionDy) {
    FLighting Total = {(FLighting)0};
    const bool Tiled = {(TileDimensions.w & 0x80000000u) == 0};
    uint2 Range = {uint2(0, TileDimensions.w & 0x7fffffffu)};

    [branch]
    if (Tiled) {
        const uint2 Tile = {min((uint2)Pixel / TileDimensions.z, TileDimensions.xy - 1)};
        Range = TileRanges[Tile.y * TileDimensions.x + Tile.x];
    }

    for (uint Offset = {0}; Offset < Range.y; ++Offset) {
        uint LightIndex = {Offset};

        [branch]
        if (Tiled) {
            LightIndex = TileLightIndices[Range.x + Offset];
        }

        const FLight Light = {Lights[LightIndex]};
        const FLighting Lighting = {EvaluateLight(Light, Position, Normal, AmbientTint, SpecularParameters)};
        float Visibility = {1.0f};

#if defined(MACAW_RECEIVE_SHADOWS)
        [branch]
        if (ReceiveShadows && Light.mShadowInfo.x >= 1.0f && (any(Lighting.mDiffuse != 0.0f) || any(Lighting.mSpecular != 0.0f))) {
            Visibility = ShadowVisibility(Light, Position, Normal, PositionDx, PositionDy, Pixel);
        }
#endif

        Total.mDiffuse += Lighting.mDiffuse * Visibility;
        Total.mSpecular += Lighting.mSpecular * Visibility;
        Total.mAmbient += Lighting.mAmbient;
    }

    return Total;
}

#endif
