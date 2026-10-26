#include "MaterialCommon.hlsli"

FOutput VertexMain(FInput Input) {
    FOutput Output = {float4(0, 0, 0, 0), float3(0, 0, 0), float2(0, 0), float4(0, 0, 0, 0)};
    Output.mPosition = mul(mul(float4(Input.mPosition, 1), World), ViewProjection);
    Output.mNormal = normalize(mul(Input.mNormal, transpose((float3x3)WorldInverse)));
    Output.mUV = Input.mUV;
    Output.mColor = Input.mColor;
    return Output;
}

float4 PixelMain(FOutput Input) : SV_TARGET {
    float4 Surface = {BaseColor * Input.mColor * BaseColorTexture.Sample(MaterialSampler, Input.mUV)};
#ifdef MACAW_UNLIT
    return Surface;
#else
    float Diffuse = {saturate(dot(normalize(Input.mNormal), normalize(float3(-0.35, -0.6, 1.0))))};
    return float4(Surface.rgb * (0.32 + 0.68 * Diffuse), Surface.a);
#endif
}
