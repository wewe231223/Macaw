#include "MaterialCommon.hlsli"

struct FLight {
    float4 mPositionAndRadius;
    float4 mDirectionAndType;
    float4 mColorAndIntensity;
    float4 mFalloffAndCones;
    float4 mShadowInfo;
};

struct FLighting {
    float3 mDiffuse;
    float3 mSpecular;
};

cbuffer LightingConstants : register(b3) {
    float4 CameraAndLightCount;
    uint4 TileDimensions;
};

StructuredBuffer<FLight> Lights : register(t2);
StructuredBuffer<uint2> TileRanges : register(t3);
StructuredBuffer<uint> TileLightIndices : register(t4);

float3 SafeNormal(float3 Value) {
    return Value * rsqrt(max(dot(Value, Value), 0.00000001));
}

#include "ShadowSampling.hlsli"

float3 MappedNormal(float3 Normal, float4 Tangent, float3 Sampled) {
    float3 N = {SafeNormal(Normal)};
    float3 T = {SafeNormal(Tangent.xyz - N * dot(N, Tangent.xyz))};
    float3 B = {cross(N, T) * Tangent.w};
    float3 Map = {Sampled * 2.0f - 1.0f};
    Map.xy *= SurfaceParameters.z;
    return SafeNormal(T * Map.x + B * Map.y + N * Map.z);
}

FLighting EvaluateLight(FLight Light, float3 Position, float3 Normal) {
    FLighting Result = {(FLighting)0};
    float3 Radiance = {Light.mColorAndIntensity.rgb * Light.mColorAndIntensity.w};
    uint Type = {(uint)Light.mDirectionAndType.w};
    if (Type == 0) {
        Result.mDiffuse = Radiance;
        return Result;
    }
    float3 L = {-Light.mDirectionAndType.xyz};
    float Attenuation = {1.0f};
    if (Type >= 2) {
        float3 Offset = {Light.mPositionAndRadius.xyz - Position};
        float Distance = {length(Offset)};
        if (Distance >= Light.mPositionAndRadius.w) {
            return Result;
        }
        L = SafeNormal(Offset);
        float Radial = {saturate(1.0f - Distance / max(Light.mPositionAndRadius.w, 0.0001f))};
        Attenuation = Light.mFalloffAndCones.x > 0.0f ? pow(Radial, Light.mFalloffAndCones.x) : 1.0f;
        if (Type == 3) {
            float Cosine = {dot(-L, Light.mDirectionAndType.xyz)};
            float Inner = {Light.mFalloffAndCones.y};
            float Outer = {Light.mFalloffAndCones.z};
            float Cone = {Inner - Outer > 0.00001f ? saturate((Cosine - Outer) / (Inner - Outer)) : step(Outer, Cosine)};
            Attenuation *= Cone * Cone * (3.0f - 2.0f * Cone);
        }
    }
    float Diffuse = {saturate(dot(Normal, L))};
    Result.mDiffuse = Radiance * (Attenuation * Diffuse);
#if defined(LIGHTING_MODEL_PHONG) || defined(LIGHTING_MODEL_GOURAUD)
    float3 V = {SafeNormal(CameraAndLightCount.xyz - Position)};
    float3 H = {SafeNormal(L + V)};
    float Specular = {Diffuse > 0.0f ? pow(saturate(dot(Normal, H)), SurfaceParameters.y) * SurfaceParameters.x : 0.0f};
    Result.mSpecular = Radiance * (Attenuation * Specular);
#endif
    return Result;
}

FOutput VertexMain(FInput Input) {
    FOutput Output = {(FOutput)0};
    Output.mWorldPosition = mul(float4(Input.mPosition, 1.0f), World).xyz;
    Output.mPosition = mul(float4(Output.mWorldPosition, 1.0f), ViewProjection);
    Output.mNormal = SafeNormal(mul(Input.mNormal, transpose((float3x3)WorldInverse)));
    Output.mTangent = float4(SafeNormal(mul(Input.mTangent.xyz, (float3x3)World)), Input.mTangent.w * (determinant((float3x3)World) < 0.0f ? -1.0f : 1.0f));
    Output.mUV = Input.mUV * UVTransform.xy + UVTransform.zw;
    Output.mColor = Input.mColor;
#if defined(LIGHTING_MODEL_GOURAUD) && !defined(MACAW_UNLIT) && !defined(WORLD_NORMAL)
    float3 Normal = {Output.mNormal};
#ifdef HAS_NORMAL_MAP
    Normal = MappedNormal(Normal, Output.mTangent, NormalMapTexture.SampleLevel(MaterialSampler, Output.mUV, 0).xyz);
#endif
    for (uint Index = {0}; Index < (uint)CameraAndLightCount.w; ++Index) {
        FLighting Lighting = {EvaluateLight(Lights[Index], Output.mWorldPosition, Normal)};
        Output.mDiffuse += Lighting.mDiffuse;
        Output.mSpecular += Lighting.mSpecular;
    }
#endif
    return Output;
}

float4 PixelMain(FOutput Input) : SV_TARGET {
    float4 Surface = {BaseColor * Input.mColor * BaseColorTexture.Sample(MaterialSampler, Input.mUV)};
    float3 Normal = {SafeNormal(Input.mNormal)};
#ifdef HAS_NORMAL_MAP
    Normal = MappedNormal(Normal, Input.mTangent, NormalMapTexture.Sample(MaterialSampler, Input.mUV).xyz);
#endif
#if defined(WORLD_NORMAL)
    return float4(Normal * 0.5f + 0.5f, 1.0f);
#elif defined(MACAW_UNLIT)
    return Surface;
#else
    FLighting Total = {(FLighting)0};
    FLighting Unshadowed = {(FLighting)0};
    float3 PositionDx = {ddx(Input.mWorldPosition)};
    float3 PositionDy = {ddy(Input.mWorldPosition)};
    uint2 Tile = {min((uint2)Input.mPosition.xy / TileDimensions.z, TileDimensions.xy - 1)};
    uint2 Range = {TileRanges[Tile.y * TileDimensions.x + Tile.x]};
    for (uint Offset = {0}; Offset < Range.y; ++Offset) {
        FLight Light = {Lights[TileLightIndices[Range.x + Offset]]};
        FLighting Lighting = {EvaluateLight(Light, Input.mWorldPosition, Normal)};
        float Visibility = {ShadowVisibility(Light, Input.mWorldPosition, Normal, PositionDx, PositionDy)};
        Unshadowed.mDiffuse += Lighting.mDiffuse;
        Unshadowed.mSpecular += Lighting.mSpecular;
        Total.mDiffuse += Lighting.mDiffuse * Visibility;
        Total.mSpecular += Lighting.mSpecular * Visibility;
    }
#if defined(LIGHTING_MODEL_GOURAUD)
    float3 DiffuseRatio = {saturate((Total.mDiffuse + 0.000001f) / (Unshadowed.mDiffuse + 0.000001f))};
    float3 SpecularRatio = {saturate((Total.mSpecular + 0.000001f) / (Unshadowed.mSpecular + 0.000001f))};
    return float4(Surface.rgb * Input.mDiffuse * DiffuseRatio + Input.mSpecular * SpecularRatio, Surface.a);
#else
    return float4(Surface.rgb * Total.mDiffuse + Total.mSpecular, Surface.a);
#endif
#endif
}
