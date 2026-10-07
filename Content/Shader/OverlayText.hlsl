Texture2D FontAtlas : register(t3);
SamplerState LinearClamp : register(s0);

#include "FrameResource.hlsli"

struct FInput {
    float2 mPosition : POSITION;
    float2 mSize : SIZE;
    float2 mUVMin : TEXCOORD0;
    float2 mUVMax : TEXCOORD1;
    float4 mColor : COLOR0;
};

struct FOutput {
    float4 mPosition : SV_POSITION;
    float2 mUV : TEXCOORD0;
    float4 mColor : COLOR0;
};

FOutput MainVS(FInput Input, uint VertexIndex : SV_VertexID) {
    const float2 Corners[6] = {float2(0.0f, 0.0f), float2(1.0f, 0.0f), float2(0.0f, 1.0f), float2(0.0f, 1.0f), float2(1.0f, 0.0f), float2(1.0f, 1.0f)};
    const float2 Corner = {Corners[VertexIndex]};
    const float2 Position = {Input.mPosition + Corner * Input.mSize};
    FOutput Output = {(FOutput)0};

    Output.mPosition = float4(Position.x * Viewport.z * 2.0f - 1.0f, 1.0f - Position.y * Viewport.w * 2.0f, 0.0f, 1.0f);
    Output.mUV = lerp(Input.mUVMin, Input.mUVMax, Corner);
    Output.mColor = Input.mColor;

    return Output;
}

float4 MainPS(FOutput Input) : SV_TARGET {
    const float Coverage = {FontAtlas.Sample(LinearClamp, Input.mUV).r};

    return float4(Input.mColor.rgb, Input.mColor.a * Coverage);
}
