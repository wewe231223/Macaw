cbuffer DecalConstants : register(b0) {
    row_major float4x4 InverseViewProjection;
    row_major float4x4 WorldToDecal;
    float4 ScreenAndOpacity;
    uint4 TextureFlags;
};

Texture2D<float4> DecalTexture : register(t0);
Texture2D<float> SceneDepth : register(t1);
Texture2D<float4> DecalNormalMap : register(t11);
Texture2D<float4> ReceiverNormals : register(t12);
Texture2D<uint2> SceneStencil : register(t13);
Texture2D<float4> AccumulatedNormals : register(t14);
SamplerState LinearClamp : register(s0);

static const uint SurfaceStencil = {2u};

float4 VertexMain(uint VertexId : SV_VertexID) : SV_POSITION {
    const float2 UV = {float2((VertexId << 1) & 2, VertexId & 2)};

    return float4(UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
}

float2 GetDecalUV(float4 Position) {
    const float Depth = {SceneDepth.Load(int3(int2(Position.xy), 0))};
    const float2 UV = {Position.xy * ScreenAndOpacity.xy};
    const float2 Ndc = {UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f)};
    const float4 World = {mul(float4(Ndc, Depth, 1.0f), InverseViewProjection)};
    const float3 Local = {mul(World, WorldToDecal).xyz / World.w};

    clip(1.0f - abs(Local));

    return Local.yz * float2(0.5f, -0.5f) + 0.5f;
}

struct FDecalOutput {
    float4 mColor : SV_TARGET0;
    float4 mNormal : SV_TARGET1;
};

float3 BlendReorientedNormals(float3 CurrentWorldNormal, float3 DetailTangentNormal, float3 Tangent, float3 Bitangent, float3 ReceiverNormal) {
    const float3 LocalBase = {float3(dot(CurrentWorldNormal, Tangent), dot(CurrentWorldNormal, Bitangent), dot(CurrentWorldNormal, ReceiverNormal))};
    const float3 Base = {dot(LocalBase, LocalBase) > 0.00000001f ? normalize(LocalBase) : float3(0.0f, 0.0f, 1.0f)};

    const float3 Pivot = {Base + float3(0.0f, 0.0f, 1.0f)};
    const float3 Reflected = {float3(-DetailTangentNormal.xy, DetailTangentNormal.z)};
    const float3 Reoriented = {Pivot * dot(Pivot, Reflected) - Reflected * Pivot.z};
    const float3 Combined = {normalize(Pivot.z < 0.00001f ? DetailTangentNormal * float3(1.0f, -1.0f, -1.0f) : Reoriented)};

    return Tangent * Combined.x + Bitangent * Combined.y + ReceiverNormal * Combined.z;
}

FDecalOutput PixelMain(float4 Position : SV_POSITION) {
    const float2 UV = {GetDecalUV(Position)};
    const float2 UvDx = {ddx(UV)};
    const float2 UvDy = {ddy(UV)};
    const int3 Texel = {int3(int2(Position.xy), 0)};
    FDecalOutput Output = {(FDecalOutput)0};
    float Opacity = {ScreenAndOpacity.z};

    if (TextureFlags.x != 0u) {
        const float4 Color = {DecalTexture.SampleGrad(LinearClamp, UV, UvDx, UvDy)};
        Opacity *= Color.a;

        if (TextureFlags.y != 0u) {
            Output.mColor = float4(Color.rgb, Opacity);
        }
    }

    [branch]
    if (Opacity <= 0.0f || ScreenAndOpacity.w <= 0.0f) {
        return Output;
    }

    if ((SceneStencil.Load(Texel).g & SurfaceStencil) == 0u) {
        return Output;
    }

    const float4 Receiver = {ReceiverNormals.Load(Texel)};

    if (Receiver.w < 0.0f || dot(Receiver.xyz, Receiver.xyz) < 0.0001f) {
        return Output;
    }

    const float3 Normal = {normalize(Receiver.xyz)};
    const float3 UAxis = {normalize(float3(WorldToDecal[0][1], WorldToDecal[1][1], WorldToDecal[2][1]))};
    const float3 VAxis = {-normalize(float3(WorldToDecal[0][2], WorldToDecal[1][2], WorldToDecal[2][2]))};
    const float Facing = {abs(dot(cross(UAxis, VAxis), Normal))};

    if (Facing < 0.0001f) {
        return Output;
    }

    const float3 Tangent = {normalize(UAxis - Normal * dot(Normal, UAxis))};
    const float3 Side = {cross(Normal, Tangent)};
    const float3 Bitangent = {dot(Side, VAxis) < 0.0f ? -Side : Side};
    const float4 Sample = {DecalNormalMap.SampleGrad(LinearClamp, UV, UvDx, UvDy)};
    const float3 Map = {Sample.xyz * 2.0f - 1.0f};
    const float Alpha = {Opacity * Sample.a * saturate(Facing * 4.0f)};

    if (Alpha <= 0.0f || (Map.z > 0.0f && all(abs(Map.xy) <= 1.0f / 255.0f + 0.00001f))) {
        return Output;
    }

    const float3 Current = {AccumulatedNormals.Load(Texel).xyz};
    const float3 Detail = {float3(Map.xy * ScreenAndOpacity.w, max(Map.z, 0.0001f))};
    const float3 Perturbed = {BlendReorientedNormals(Current, Detail, Tangent, Bitangent, Normal)};

    Output.mNormal = float4(Perturbed, Alpha);

    return Output;
}
