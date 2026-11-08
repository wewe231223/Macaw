#define LIGHTING_MODEL_PHONG 1
#define MACAW_RECEIVE_SHADOWS 1
#include "PixelLighting.hlsli"

cbuffer DeferredConstants : register(b0) {
    row_major float4x4 InverseViewProjection;
    float2 InverseSize;
    uint ViewMode;
    uint Padding;
};

Texture2D<float4> SurfaceColor : register(t11);
Texture2D<float4> SurfaceNormal : register(t12);
Texture2D<float4> SurfaceSpecular : register(t13);
Texture2D<float4> SurfaceEmission : register(t14);
Texture2D<float4> SurfaceAmbient : register(t15);
Texture2D<float4> SurfaceReflection : register(t16);
Texture2D<float> SceneDepth : register(t17);
Texture2D<uint2> SceneStencil : register(t18);

static const uint ReceivesDecalsStencil = {1};
static const uint SurfaceStencil = {2};
static const uint PrelitStencil = {4};
static const uint GouraudView = {2};
static const uint LambertView = {3};
static const uint UnlitView = {4};
static const uint WorldNormalView = {5};
static const uint BaseColorView = {7};
static const uint SpecularView = {8};
static const uint EmissiveView = {9};
static const uint AmbientColorView = {10};
static const uint ReflectionView = {11};
static const uint BaseColorAlphaView = {12};
static const uint ShininessView = {13};
static const uint ReceiveShadowsView = {14};
static const uint SceneStencilView = {16};

float4 VertexMain(uint VertexId : SV_VertexID) : SV_POSITION {
    const float2 UV = {float2((VertexId << 1) & 2, VertexId & 2)};

    return float4(UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
}

float3 ReconstructWorld(float2 Ndc, float Depth) {
    const float4 World = {mul(float4(Ndc, Depth, 1.0f), InverseViewProjection)};

    return World.xyz / World.w;
}

float3 VisualizeMaterialBuffer(int3 Texel) {
    switch (ViewMode) {
    case BaseColorView:
        return SurfaceColor.Load(Texel).rgb;
    case SpecularView:
        return SurfaceSpecular.Load(Texel).rgb;
    case EmissiveView:
        return SurfaceEmission.Load(Texel).rgb;
    case AmbientColorView:
        return SurfaceAmbient.Load(Texel).rgb;
    case ReflectionView:
        return SurfaceReflection.Load(Texel).rgb;
    case BaseColorAlphaView:
        return SurfaceColor.Load(Texel).a;
    case ShininessView:
        return log2(max(SurfaceNormal.Load(Texel).w, 1.0f)) / 10.0f;
    case ReceiveShadowsView:
        return SurfaceSpecular.Load(Texel).w;
    default:
        return 0.0f;
    }
}

float4 PixelMain(float4 Pixel : SV_POSITION) : SV_TARGET {
    const int3 Texel = {int3(int2(Pixel.xy), 0)};
    const uint Stencil = {SceneStencil.Load(Texel).g};

    if (ViewMode == SceneStencilView) {
        return float4((Stencil & ReceivesDecalsStencil) != 0, (Stencil & SurfaceStencil) != 0, (Stencil & PrelitStencil) != 0, 1.0f);
    }

    if (ViewMode >= BaseColorView && ViewMode <= ReceiveShadowsView) {
        if ((Stencil & SurfaceStencil) == 0) {
            discard;
        }

        return float4(VisualizeMaterialBuffer(Texel), 1.0f);
    }

    if ((Stencil & (SurfaceStencil | PrelitStencil)) == 0) {
        discard;
    }

    const float4 Color = {SurfaceColor.Load(Texel)};

    if ((Stencil & PrelitStencil) != 0 || ViewMode == UnlitView) {
        return Color;
    }

    const float4 NormalData = {SurfaceNormal.Load(Texel)};
    const float3 Normal = {SafeNormal(NormalData.xyz)};

    if (ViewMode == WorldNormalView) {
        return float4(Normal * 0.5f + 0.5f, 1.0f);
    }

    if (NormalData.w < 0.0f) {
        return Color;
    }

    const float4 Emission = {SurfaceEmission.Load(Texel)};
    const float4 Ambient = {SurfaceAmbient.Load(Texel)};
    if (ViewMode == GouraudView) {
        return float4(Color.rgb * Ambient.rgb + Emission.rgb, Color.a);
    }

    const float4 Specular = {SurfaceSpecular.Load(Texel)};
    const float4 Reflection = {SurfaceReflection.Load(Texel)};
    const float Depth = {SceneDepth.Load(Texel)};
    const float2 Ndc = {Pixel.xy * InverseSize * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f)};
    const float3 Position = {ReconstructWorld(Ndc, Depth)};
    const bool ReceiveShadows = {Specular.w > 0.0f};
    const float2 SpecularParameters = {float2(ViewMode == LambertView ? 0.0f : 1.0f, NormalData.w)};
    const FLighting Total = {AccumulatePixelLighting(Pixel.xy, Position, Normal, Ambient.rgb, SpecularParameters, ReceiveShadows)};
    const float3 Lit = {Color.rgb * Total.mDiffuse + Total.mSpecular * Specular.rgb + Emission.rgb + Total.mAmbient * Reflection.rgb};

    return float4(Lit, Color.a);
}
