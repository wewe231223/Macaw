#include "ViewConstants.hlsli"
struct FInput {
    float3 mPosition : POSITION;
    float4 mColor : COLOR;
};
struct FOutput {
    float4 mPosition : SV_POSITION;
    float4 mColor : COLOR;
};
FOutput VertexMain(FInput Input) {
    FOutput Output = {float4(0, 0, 0, 0), float4(0, 0, 0, 0)};
    Output.mPosition = mul(float4(Input.mPosition, 1), ViewProjection);
    Output.mColor = Input.mColor;
    return Output;
}
float4 PixelMain(FOutput Input) : SV_TARGET {
    return Input.mColor;
}
