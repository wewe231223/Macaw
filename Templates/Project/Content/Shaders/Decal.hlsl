cbuffer DecalConstants : register(b0) {
    row_major float4x4 InverseViewProjection;
    row_major float4x4 WorldToDecal;
    float4 ScreenAndOpacity;
};

Texture2D<float4> DecalTexture : register(t0);
Texture2D<float> SceneDepth : register(t1);
SamplerState LinearClamp : register(s0);

float4 VertexMain(uint VertexId : SV_VertexID) : SV_POSITION {
    const float2 UV = {float2((VertexId << 1) & 2, VertexId & 2)};

    return float4(UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
}

float4 PixelMain(float4 Position : SV_POSITION) : SV_TARGET {
    const float Depth = {SceneDepth.Load(int3(int2(Position.xy), 0))};
    const float2 UV = {Position.xy * ScreenAndOpacity.xy};
    const float2 Ndc = {UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f)};
    const float4 World = {mul(float4(Ndc, Depth, 1.0f), InverseViewProjection)};
    const float3 Local = {mul(World, WorldToDecal).xyz / World.w};

    clip(1.0f - abs(Local));

    float4 Color = {DecalTexture.Sample(LinearClamp, Local.yz * float2(0.5f, -0.5f) + 0.5f)};
    Color.a *= ScreenAndOpacity.z;

    return Color;
}
