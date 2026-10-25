cbuffer EditorMeshConstants : register(b0) {
    row_major float4x4 World;
    float4 Color;
};
#include "ViewConstants.hlsli"
struct FInput {
    float3 mPosition : POSITION;
};
struct FOutput {
    float4 mPosition : SV_POSITION;
};
FOutput VertexMain(FInput Input) {
    FOutput Output = {float4(0, 0, 0, 0)};
    Output.mPosition = mul(mul(float4(Input.mPosition, 1), World), ViewProjection);
    return Output;
}
float4 PixelMain() : SV_TARGET {
    return Color;
}
