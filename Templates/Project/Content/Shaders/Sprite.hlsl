#pragma pack_matrix(row_major)

cbuffer SpriteConstants : register(b0) {
    float4x4 Transform;
    float4 UVTransform;
    float4 Color;
};

Texture2D SpriteTexture : register(t0);
SamplerState SpriteSampler : register(s0);

struct FOutput {
    float4 mPosition : SV_POSITION;
    float2 mUV : TEXCOORD0;
};

FOutput VertexMain(uint Vertex : SV_VertexID) {
    const float2 Corners[6] = {float2(-0.5f, 0.5f), float2(-0.5f, -0.5f), float2(0.5f, 0.5f), float2(0.5f, 0.5f), float2(-0.5f, -0.5f), float2(0.5f, -0.5f)};
    FOutput Output = (FOutput)0;
    Output.mPosition = mul(float4(Corners[Vertex], 0.0f, 1.0f), Transform);
    Output.mUV = (Corners[Vertex] * float2(1.0f, -1.0f) + 0.5f) * UVTransform.xy + UVTransform.zw;
    return Output;
}

float4 PixelMain(FOutput Input) : SV_TARGET {
    float4 Surface = SpriteTexture.Sample(SpriteSampler, Input.mUV) * Color;
    clip(Surface.a - 0.001f);
    return Surface;
}
