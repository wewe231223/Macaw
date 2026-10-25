cbuffer ObjectConstants : register(b0) {
    row_major float4x4 World;
    row_major float4x4 WorldInverse;
    float4 Color;
};
#include "ViewConstants.hlsli"
struct FInput {
    float3 mPosition : POSITION;
    float3 mNormal : NORMAL;
    float4 mColor : COLOR;
};
struct FOutput {
    float4 mPosition : SV_POSITION;
    float3 mNormal : NORMAL;
    float4 mColor : COLOR;
};
FOutput VertexMain(FInput Input) {
    FOutput Output = {float4(0, 0, 0, 0), float3(0, 0, 0), float4(0, 0, 0, 0)};
    Output.mPosition = mul(mul(float4(Input.mPosition, 1), World), ViewProjection);
    Output.mNormal = normalize(mul(Input.mNormal, transpose((float3x3)WorldInverse)));
    Output.mColor = Input.mColor;
    return Output;
}
float4 PixelMain(FOutput Input) : SV_TARGET {
    float Diffuse = {saturate(dot(normalize(Input.mNormal), normalize(float3(-0.35, -0.6, 1.0))))};
    return float4(Color.rgb * Input.mColor.rgb * (0.32 + 0.68 * Diffuse), Color.a * Input.mColor.a);
}
