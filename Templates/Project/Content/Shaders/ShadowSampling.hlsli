struct FShadowMap {
    row_major float4x4 mViewProjection;
    float4 mAtlasRectangle;
    float4 mParameters;
    float4 mCascade;
};

Texture2D<float> ShadowAtlas : register(t5);
StructuredBuffer<FShadowMap> ShadowMaps : register(t6);
SamplerComparisonState ShadowSampler : register(s1);

cbuffer ShadowViewConstants : register(b4) {
    float4 ShadowCamera;
    float4 ShadowForward;
};

float SampleShadowMap(uint Index, float3 Position, float3 Normal, float3 LightDirection, float3 PositionDx, float3 PositionDy) {
    FShadowMap Map = {ShadowMaps[Index]};
    float4 Clip = {mul(float4(Position, 1.0f), Map.mViewProjection)};
    if (Clip.w <= 0.0f) {
        return 1.0f;
    }
    float3 Projected = {Clip.xyz / Clip.w};
    float2 UV = {Projected.xy * float2(0.5f, -0.5f) + 0.5f};
    if (any(UV < 0.0f) || any(UV > 1.0f) || Projected.z < 0.0f || Projected.z > 1.0f) {
        return 1.0f;
    }
    float Cosine = {saturate(dot(Normal, LightDirection))};
    float Slope = {min(sqrt(saturate(1.0f - Cosine * Cosine)) / max(Cosine, 0.05f), 16.0f)};
    float4 ClipDx = {mul(float4(Position + PositionDx, 1.0f), Map.mViewProjection)};
    float4 ClipDy = {mul(float4(Position + PositionDy, 1.0f), Map.mViewProjection)};
    float3 ProjectedDx = {ClipDx.xyz / ClipDx.w - Projected};
    float3 ProjectedDy = {ClipDy.xyz / ClipDy.w - Projected};
    float2 UVdx = {ProjectedDx.xy * float2(0.5f, -0.5f)};
    float2 UVdy = {ProjectedDy.xy * float2(0.5f, -0.5f)};
    float Depthdx = {ProjectedDx.z};
    float Depthdy = {ProjectedDy.z};
    float Determinant = {UVdx.x * UVdy.y - UVdx.y * UVdy.x};
    float2 Gradient = {abs(Determinant) > 0.00000001f ? float2(Depthdx * UVdy.y - Depthdy * UVdx.y, Depthdy * UVdx.x - Depthdx * UVdy.x) / Determinant : 0.0f};
    float ReceiverSlope = {(abs(Gradient.x) + abs(Gradient.y)) * Map.mParameters.w / Map.mAtlasRectangle.z};
    float Bias = {Map.mParameters.x + Map.mParameters.y * max(ReceiverSlope * 1.5f, Slope * 0.0001f)};
    float2 AtlasUV = {Map.mAtlasRectangle.xy + UV * Map.mAtlasRectangle.zw};
    float Texel = {Map.mParameters.w};
    float2 MinimumUV = {Map.mAtlasRectangle.xy + Texel * 0.5f};
    float2 MaximumUV = {Map.mAtlasRectangle.xy + Map.mAtlasRectangle.zw - Texel * 0.5f};
    float Visibility = {0.0f};
    [unroll]
    for (int Y = {-1}; Y <= 1; ++Y) {
        [unroll]
        for (int X = {-1}; X <= 1; ++X) {
            Visibility += ShadowAtlas.SampleCmpLevelZero(ShadowSampler, clamp(AtlasUV + float2(X, Y) * Texel, MinimumUV, MaximumUV), Projected.z - Bias);
        }
    }
    Visibility /= 9.0f;
    return saturate((Visibility - 0.5f) * (1.0f + Map.mParameters.z) + 0.5f);
}

float ShadowVisibility(FLight Light, float3 Position, float3 Normal, float3 PositionDx, float3 PositionDy) {
    if (Light.mShadowInfo.x < 1.0f || ShadowFlags.x < 0.5f) {
        return 1.0f;
    }
    uint Start = {(uint)Light.mShadowInfo.x - 1};
    uint Count = {(uint)Light.mShadowInfo.y};
    uint Type = {(uint)Light.mDirectionAndType.w};
    float3 L = {Type >= 2 ? SafeNormal(Light.mPositionAndRadius.xyz - Position) : -Light.mDirectionAndType.xyz};
    if (Type == 1) {
        float Depth = {dot(Position - ShadowCamera.xyz, ShadowForward.xyz)};
        for (uint Cascade = {0}; Cascade < Count; ++Cascade) {
            FShadowMap Map = {ShadowMaps[Start + Cascade]};
            if (Depth <= Map.mCascade.y) {
                float Current = {SampleShadowMap(Start + Cascade, Position, Normal, L, PositionDx, PositionDy)};
                if (Depth > Map.mCascade.z && Map.mCascade.y > Map.mCascade.z) {
                    float Next = {Cascade + 1 < Count ? SampleShadowMap(Start + Cascade + 1, Position, Normal, L, PositionDx, PositionDy) : 1.0f};
                    float Blend = {smoothstep(Map.mCascade.z, Map.mCascade.y, Depth)};
                    return lerp(Current, Next, Blend);
                }
                return Current;
            }
        }
        return 1.0f;
    }
    uint Face = {0};
    if (Type == 2) {
        float3 Offset = {Position - Light.mPositionAndRadius.xyz};
        float3 Magnitude = {abs(Offset)};
        if (Magnitude.x >= Magnitude.y && Magnitude.x >= Magnitude.z) {
            Face = Offset.x >= 0.0f ? 0 : 1;
        } else if (Magnitude.y >= Magnitude.z) {
            Face = Offset.y >= 0.0f ? 2 : 3;
        } else {
            Face = Offset.z >= 0.0f ? 4 : 5;
        }
    }
    return SampleShadowMap(Start + Face, Position, Normal, L, PositionDx, PositionDy);
}
