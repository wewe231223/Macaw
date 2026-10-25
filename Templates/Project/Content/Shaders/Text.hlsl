#pragma pack_matrix(row_major)

cbuffer TextConstants : register(b0) {
    float4x4 Transform;
    float4 Color;
    float4 Parameters;
};

struct FVertexInput {
    float2 Position : POSITION;
    float2 UV : TEXCOORD0;
};

struct FPixelInput {
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

Texture2D FontAtlas : register(t0);
SamplerState FontSampler : register(s0);

FPixelInput VertexMain(FVertexInput Input) {
    FPixelInput Output = (FPixelInput)0;
    Output.Position = mul(float4(Input.Position, 0.0f, 1.0f), Transform);
    Output.UV = Input.UV;
    return Output;
}

float4 PixelMain(FPixelInput Input) : SV_TARGET {
    float3 Distance = FontAtlas.Sample(FontSampler, Input.UV).rgb;
    float Median = max(min(Distance.r, Distance.g), min(max(Distance.r, Distance.g), Distance.b));
    float2 UnitRange = Parameters.xx / Parameters.yz;
    float2 ScreenSize = 1.0f / max(fwidth(Input.UV), float2(0.000001f, 0.000001f));
    float PixelRange = max(0.5f * dot(UnitRange, ScreenSize), 1.0f);
    float Opacity = saturate(PixelRange * (Median - 0.5f) + 0.5f);
    clip(Opacity * Color.a - 0.001f);
    return float4(Color.rgb, Color.a * Opacity);
}
